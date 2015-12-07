namespace sbmt {
namespace logmath {

template <class BaseT,class F>
typename ln_base<BaseT,F>::lnbase_type
ln_base<BaseT,F>::lnbase;

template <class from_base,class to_base,class F>
typename log_base_convert<from_base,to_base,F>::converter_type
log_base_convert<from_base,to_base,F>::converter;

template <class BaseT,class F>
typename logmath_compute<BaseT,F> ::order_type
logmath_compute<BaseT,F>::order;

}
}
