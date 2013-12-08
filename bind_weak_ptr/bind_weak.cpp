
#include <tuple>
#include <memory>
#include <utility>
#include <iostream>
#include <type_traits>

/**
 * Stores a tuple of indices. Used by bind() to extract the elements
 * in a tuple. 
 */
template<int... _Indexes>
struct _Index_tuple
{
    typedef _Index_tuple<_Indexes..., sizeof...(_Indexes)> __next;
};

/// Builds an _Index_tuple<0, 1, 2, ..., _Num-1>.
template<std::size_t _Num>
struct _Build_index_tuple
{
    typedef typename _Build_index_tuple<_Num-1>::__type::__next __type;
};

template<>
struct _Build_index_tuple<0>
{
    typedef _Index_tuple<> __type;
};

template<typename _Functor>
class _Bind;

template<typename _Functor, typename _Class, typename... _Bound_args>
class _Bind<_Functor(_Class, _Bound_args...)>
{
    typedef _Bind __self_type;
    typedef typename _Build_index_tuple<sizeof...(_Bound_args)>::__type
        _Bound_indexes;
    typedef std::weak_ptr<_Class> _WeakPtr;

    _Functor _M_f;
    _WeakPtr _M_w_ptr;

    std::tuple<_Bound_args...> _M_bound_args;

    // Call unqualified
    template<typename... _Args, int... _Indexes>
    void __call(_Index_tuple<_Indexes...>, _Args&&... __args)
    {
        if (auto ptr = _M_w_ptr.lock()) {
            _M_f(ptr, std::get<_Indexes>(_M_bound_args)..., std::forward<_Args>(__args)...);
        }
    }

    public:

    template<typename... _Args>
    explicit _Bind(const _Functor& __f, _WeakPtr __w_ptr, _Args&&... __args)
    : _M_f(__f), _M_w_ptr(__w_ptr), _M_bound_args(std::forward<_Args>(__args)...)
    { }

    template<typename... _Args>
    explicit _Bind(_Functor&& __f, _WeakPtr __w_ptr, _Args&&... __args)
     : _M_f(std::move(__f)), _M_w_ptr(__w_ptr), _M_bound_args(std::forward<_Args>(__args)...)
    { }

    _Bind(const _Bind&) = default;

    _Bind(_Bind&& __b)
    : _M_f(std::move(__b._M_f)), _M_w_ptr(std::move(__b._M_w_ptr)), _M_bound_args(std::move(__b._M_bound_args))
    { }

    // Call unqualified
    template<typename... _Args>
    void operator()(_Args&&... __args)
    {
        __call(_Bound_indexes(), std::forward<_Args>(__args)...);
    }
};



  template<typename _MemberPointer>
    class _Mem_fn;

  /**
   * Derives from @c unary_function or @c binary_function, or perhaps
   * nothing, depending on the number of arguments provided. The
   * primary template is the basis case, which derives nothing.
   */
  template<typename _Res, typename... _ArgTypes>
    struct _Maybe_unary_or_binary_function { };

  /// Derives from @c unary_function, as appropriate.
  template<typename _Res, typename _T1>
    struct _Maybe_unary_or_binary_function<_Res, _T1>
    : std::unary_function<_T1, _Res> { };

  /// Derives from @c binary_function, as appropriate.
  template<typename _Res, typename _T1, typename _T2>
    struct _Maybe_unary_or_binary_function<_Res, _T1, _T2>
    : std::binary_function<_T1, _T2, _Res> { };

  /// Implementation of @c mem_fn for member function pointers.
  template<typename _Res, typename _Class, typename... _ArgTypes>
    class _Mem_fn<_Res (_Class::*)(_ArgTypes...)>
    : public _Maybe_unary_or_binary_function<_Res, _Class*, _ArgTypes...>
    {
      typedef _Res (_Class::*_Functor)(_ArgTypes...);

      template<typename _Tp>
	_Res
	_M_call(_Tp& __object, const volatile _Class *,
		_ArgTypes... __args) const
	{ return (__object.*__pmf)(std::forward<_ArgTypes>(__args)...); }

      template<typename _Tp>
	_Res
	_M_call(_Tp& __ptr, const volatile void *, _ArgTypes... __args) const
	{ return ((*__ptr).*__pmf)(std::forward<_ArgTypes>(__args)...); }

    public:
      typedef _Res result_type;

      explicit _Mem_fn(_Functor __pmf) : __pmf(__pmf) { }

      // Handle objects
      _Res
      operator()(_Class& __object, _ArgTypes... __args) const
      { return (__object.*__pmf)(std::forward<_ArgTypes>(__args)...); }

      // Handle pointers
      _Res
      operator()(_Class* __object, _ArgTypes... __args) const
      { return (__object->*__pmf)(std::forward<_ArgTypes>(__args)...); }

      // Handle smart pointers, references and pointers to derived
      template<typename _Tp>
	_Res
	operator()(_Tp& __object, _ArgTypes... __args) const
	{
	  return _M_call(__object, &__object,
	      std::forward<_ArgTypes>(__args)...);
	}

    private:
      _Functor __pmf;
    };


/**
 *  Maps member pointers into instances of _Mem_fn but leaves all
 *  other function objects untouched. Used by tr1::bind(). The
 *  primary template handles the non--member-pointer case.
 */
template<typename _Tp>
struct _Maybe_wrap_member_pointer
{
    typedef _Tp type;

    static const _Tp&
        __do_wrap(const _Tp& __x)
        { return __x; }

    static _Tp&&
        __do_wrap(_Tp&& __x)
        { return static_cast<_Tp&&>(__x); }
};

/**
 *  Maps member pointers into instances of _Mem_fn but leaves all
 *  other function objects untouched. Used by tr1::bind(). This
 *  partial specialization handles the member pointer case.
 */
template<typename _Tp, typename _Class>
struct _Maybe_wrap_member_pointer<_Tp _Class::*>
{
    typedef _Mem_fn<_Tp _Class::*> type;

    static type
        __do_wrap(_Tp _Class::* __pm)
        { return type(__pm); }
};



template<typename _Functor, typename... _ArgTypes>
struct _Bind_helper
{
    typedef _Maybe_wrap_member_pointer<typename std::decay<_Functor>::type>
        __maybe_type;
    typedef typename __maybe_type::type __functor_type;
    typedef _Bind<__functor_type(typename std::decay<_ArgTypes>::type...)> type;
};


template<typename _Functor, typename _Class, typename... _ArgTypes>
inline
typename _Bind_helper<_Functor, _Class, _ArgTypes...>::type
bind(_Functor&& __f, std::weak_ptr<_Class> __w_ptr, _ArgTypes&&... __args)
{
    typedef _Bind_helper<_Functor, _Class, _ArgTypes...> __helper_type;
    typedef typename __helper_type::__maybe_type __maybe_type;
    typedef typename __helper_type::type __result_type;
    return __result_type(__maybe_type::__do_wrap(std::forward<_Functor>(__f)),
                         __w_ptr,
                         std::forward<_ArgTypes>(__args)...);
}

struct A {
    void test(int a)
    {
        std::cout << a << std::endl;
    }
};

int main()
{
    std::shared_ptr<A> ptr = std::make_shared<A>();

    auto b_ptr = bind(&A::test,
                      std::weak_ptr<A>(ptr));
    b_ptr(10);
    auto c_ptr = bind(&A::test,
                      std::weak_ptr<A>(ptr),
                      20);
    c_ptr();

    auto d_ptr = bind(&A::test,
                      std::weak_ptr<A>(ptr),
                      30);
    ptr.reset();
    d_ptr();

#if 0
   auto b_ptr = _Bind<(A::*)(std::weak_ptr<A>, int)>(&A::test,
                                                   std::weak_ptr<A>(ptr),
                                                   10);
#endif
}

