/*
 * main.cpp
 *
 *  Created on: Jul 16, 2013
 *      Author: ykirpichev
 */

#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <stdexcept>
#include <unordered_map>

#include "boost/program_options.hpp"
#include "boost/functional/hash.hpp"
#include "zlib.h"

using namespace std;
namespace po = boost::program_options;

const long SPAN = 32768L; /* desired distance between access points */
const unsigned WINSIZE = 32768U; /* sliding window size */
const unsigned CHUNK = WINSIZE; /* file input buffer size */
const string CHECK_POINT = "point.tmp";

off_t totin, totout;
off_t last;

class compression_error : public std::runtime_error {
public:
    explicit compression_error(const std::string& msg) : std::runtime_error(msg) {
    }
    explicit compression_error(const char* msg) : std::runtime_error(msg) {
    }
};

enum OperationMode {
    Compress,
    Decompress,
    Decompress_Continue,
};

std::size_t hash_value(const OperationMode& m)
{
    boost::hash<int> hasher;
    return hasher(static_cast<int>(m));
}

ostream& operator<<(ostream& os, const OperationMode& mode)
{
    const std::unordered_map<OperationMode, string, boost::hash<OperationMode>> cmap = {
        {Compress, "Compress"},
        {Decompress, "Decompress"},
        {Decompress_Continue, "Decompress_Continue"}
    };
    return os << cmap.at(mode);
}

istream& operator>>(istream& is, OperationMode& mode)
{
    const std::unordered_map<string, OperationMode> cmap = {
        {"Compress", Compress},
        {"Decompress", Decompress},
        {"Decompress_Continue", Decompress_Continue},
        {"0", Compress},
        {"1", Decompress},
        {"2", Decompress_Continue},
    };
    string tmp;

    is >> tmp;
    mode = cmap.at(tmp);

    return is;
}

/* access point entry */
struct CheckPoint {
    off_t out; /* corresponding offset in uncompressed data */
    off_t in; /* offset in input file of first full byte */
    int bits; /* number of bits (1-7) from byte at in - 1, or 0 */
    char window[WINSIZE]; /* preceding 32K of uncompressed data */
} check_point;

ostream& operator<<(ostream& os, const CheckPoint& cpoint)
{
    os << cpoint.out << " ";
    os << cpoint.in << " ";
    os << cpoint.bits << endl;

    os.write(cpoint.window, WINSIZE);

    return os;
}

istream& operator>>(istream& is, CheckPoint& cpoint)
{
    is >> cpoint.out;
    is >> cpoint.in;
    is >> cpoint.bits;
    is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    is.read(cpoint.window, WINSIZE);

    return is;
}

void save_point()
{
    cout << "save_point: " << check_point.out << " " << check_point.in
        << " " << check_point.bits << endl;

    ofstream(CHECK_POINT) << check_point;
}

void restore_point()
{
    ifstream(CHECK_POINT) >> check_point;
    cout << "restore_point: " << check_point.out << " " << check_point.in
        << " " << check_point.bits << endl;
}
/* Add an entry to the access point list. If out of memory, deallocate the
   existing list and return NULL. */
void keep_point(int bits, off_t in, off_t out, unsigned left,
                char *window)
{
    /* fill in entry and increment how many we have */

    //	cout << "keep_point was called : " << bits << " " << in << " " << out
    //			<< " " << left << endl;
    cout << "." << flush;

    check_point.bits = bits;
    check_point.in = in;
    check_point.out = out;

    if (left)
        memcpy(check_point.window, window + WINSIZE - left, left);

    if (left < WINSIZE)
        memcpy(check_point.window + left, window, WINSIZE - left);
}

int compress(ifstream *source, ofstream *dest, int level)
{
    int ret, flush;
    off_t have;
    z_stream strm;
    char in[CHUNK];
    char out[WINSIZE];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        //strm.avail_in =
        source->read(in, CHUNK);
        strm.avail_in = source->gcount();

        if (!source->good() && !source->eof()) {
            (void) deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = source->eof() ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = reinterpret_cast<unsigned char*>(in);

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = WINSIZE;
            strm.next_out = reinterpret_cast<unsigned char*>(out);

            ret = deflate(&strm, flush); /* no bad return value */
            assert(ret != Z_STREAM_ERROR); /* state not clobbered */

            have = WINSIZE - strm.avail_out;
            dest->write(out, have);
            if (!dest->good()) {
                (void) deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0); /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END); /* stream will be complete */

    /* clean up and return */
    (void) deflateEnd(&strm);
    return Z_OK;
}


/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int decompress_loop(ifstream* source, ofstream* dest,
                    z_stream* stream)
{
    char in[CHUNK] = {0};
    char out[WINSIZE] = {0};
    int ret;
    off_t have;

    /* decompress until deflate stream ends or end of file */
    do {
        source->read(in, sizeof(in));
        stream->avail_in = source->gcount();
        if (!source->good() && !source->eof()) {
            return Z_ERRNO;
        }
        if (stream->avail_in == 0)
            break;

        stream->next_in = reinterpret_cast<unsigned char*>(in);
        /* run inflate() on input until output buffer not full */
        do {
            /* reset sliding window if necessary */
            if (stream->avail_out == 0) {
                stream->avail_out = sizeof(out);
                stream->next_out = reinterpret_cast<unsigned char*>(out);
            }

            /* inflate until out of input, output, or at end of block --
               update the total input and output counters */
            totin += stream->avail_in;
            have = totout;
            totout += stream->avail_out;

            ret = inflate(stream, Z_BLOCK); /* return at end of block */
            totin -= stream->avail_in;
            totout -= stream->avail_out;
            have = totout - have;

            assert(ret != Z_STREAM_ERROR); /* state not clobbered */
            if (ret == Z_NEED_DICT)
                ret = Z_DATA_ERROR;
            if (ret == Z_MEM_ERROR || ret == Z_DATA_ERROR)
                return ret;
            if (ret == Z_STREAM_END)
                break;

            /* if at end of block, consider adding an index entry (note that if
               data_type indicates an end-of-block, then all of the
               uncompressed data from that block has been delivered, and none
               of the compressed data after that block has been consumed,
               except for up to seven bits) -- the totout == 0 provides an
               entry point after the zlib or gzip header, and assures that the
               index always has at least one access point; we avoid creating an
               access point after the last block by checking bit 6 of data_type
               */
            if ((stream->data_type & 128) && !(stream->data_type & 64) &&
                (totout == 0 || totout - last > SPAN)) {
                keep_point(stream->data_type & 7, totin, totout, stream->avail_out, out);
                last = totout;
            }

            dest->write(out + WINSIZE - stream->avail_out - have, have);
            if (!dest->good()) {
                return Z_ERRNO;
            }
        } while (stream->avail_in != 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    return ret;
}

int decompress(ifstream *source, ofstream *dest)
{
    int ret;
    z_stream strm;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.avail_out = 0;
    strm.next_in = Z_NULL;

    ret = inflateInit(&strm);
    if (ret != Z_OK)
        throw compression_error("inflateInit(&strm); failed");

    /* decompress until deflate stream ends or end of file */
    ret = decompress_loop(source, dest, &strm);

    /* clean up and return */
    (void) inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : (throw compression_error("decompress_loop error"), Z_DATA_ERROR);
}

int decompress_continue(ifstream *source, ofstream *dest, bool ignore_read_offset)
{
    int ret;
    z_stream strm;

    /* initialize file and inflate state to start there */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.avail_out = 0;
    strm.next_in = Z_NULL;
    /*
     * windowBits can also be -8..-15 for raw inflate.  In this case, -windowBits
     * determines the window size.  inflate() will then process raw deflate data,
     * not looking for a zlib or gzip header, not generating a check value, and not
     * looking for any check values for comparison at the end of the stream.  This
     * is for use with other formats that use the deflate compressed data format
     * such as zip.  Those formats provide their own check values.  If a custom
     * format is developed using the raw deflate format for compressed data, it is
     * recommended that a check value such as an adler32 or a crc32 be applied to
     * the uncompressed data as is done in the zlib, gzip, and zip formats.  For
     * most applications, the zlib format should be used as is.  Note that comments
     * above on the use in deflateInit2() applies to the magnitude of windowBits.
     */
    ret = inflateInit2(&strm, -15); /* raw inflate */
    if (ret != Z_OK)
        throw compression_error("inflateInit2(&strm, -15) failed");

    totin = check_point.in;
    totout = check_point.out;

    if (!ignore_read_offset) {
        cout << "continue: move read pointer to appropriate place" << endl;
        source->seekg(check_point.in - (check_point.bits ? 1 : 0));
    }

    dest->seekp(check_point.out);

    if (check_point.bits) {
        char tmp;
        source->read(&tmp, sizeof(tmp));

        inflatePrime(&strm, check_point.bits, tmp >> (8 - check_point.bits));
    }

    inflateSetDictionary(&strm, reinterpret_cast<unsigned char*>(check_point.window), WINSIZE);

    ret = decompress_loop(source, dest, &strm);

    /* clean up and return */
    (void) inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : (throw compression_error("decompress_loop error"), Z_DATA_ERROR);
}


int main(int argc, char **argv)
{
    // Declare the supported options.
    OperationMode mode;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("mode,m", po::value<OperationMode>(&mode)->default_value(Compress), "set working mode")
        ("input-file,i", po::value<string>(), "input file")
        ("output-file,o",po::value<string>(), "output file")
        ("ignore-read-offset,f", "in case of Decompress_Continue, "
         "specify whether ignore read offset");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (std::exception& ex) {
        cout << "parsing command line error : " << ex.what() << endl;
        return 1;
    }

    if (vm.count("help")) {
        cout << desc << endl;
        return 1;
    }

    try {
        ifstream ifs(vm["input-file"].as<string>());
        ofstream ofs;

        switch (mode) {
            case Compress:
                ofs.open(vm["output-file"].as<string>(), std::ios::binary);
                compress(&ifs, &ofs, Z_BEST_COMPRESSION);
                break;

            case Decompress:
                ofs.open(vm["output-file"].as<string>(), std::ios::binary);
                decompress(&ifs, &ofs);
                break;

            case Decompress_Continue:
                ofs.open(vm["output-file"].as<string>(), std::ios::binary | std::ios::in);
                restore_point();
                decompress_continue(&ifs, &ofs, vm.count("ignore-read-offset"));
                break;

            default:
                throw std::runtime_error("invalid operation mode");
        }
    } catch (boost::bad_any_cast& ex) {
        cout << endl;
        cout << "incorrect option was specified" << endl;
        cout << desc << endl;
    } catch (compression_error& ex) {
        cout << endl;
        save_point();
        cout << "Warning: error has happened: " << ex.what() << endl;
        cout << "CheckPoint was stored, you may proceed from check point" << endl;
    }
    cout << endl;
}

