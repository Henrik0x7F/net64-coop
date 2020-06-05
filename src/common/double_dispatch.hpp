
//
// Created by henrik on 17.11.19.
// Copyright 2020 Net64 Coop Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include <type_traits>
#include <typeinfo>


namespace Impl
{
struct TypeIdPredicate
{
    template<typename Concrete, typename Base>
    bool check(const Base& base)
    {
        return (typeid(base) == typeid(Concrete));
    }
};

struct DynamicCastPredicate
{
    template<typename Concrete, typename Base>
    bool check(const Base& base)
    {
        return (dynamic_cast<const Concrete*>(&base) != nullptr);
    }
};

template<typename Base, typename Concrete>
struct AddQualifiers;

template<typename Base, typename Concrete>
struct AddQualifiers<Base*, Concrete>
{
    using type = Concrete*;
};
template<typename Base, typename Concrete>
struct AddQualifiers<Base&, Concrete>
{
    using type = Concrete&;
};
template<typename Base, typename Concrete>
struct AddQualifiers<const Base*, Concrete>
{
    using type = const Concrete*;
};

template<typename Base, typename Concrete>
struct AddQualifiers<const Base&, Concrete>
{
    using type = const Concrete&;
};

template<typename T>
T& deref(T* v)
{
    return *v;
}

template<typename T>
T& deref(T& v)
{
    return v;
}

template<typename Handler, typename Predicate, typename BaseType, typename ConcreteType, typename... ConcreteTypes>
bool double_dispatch(Handler handler, BaseType param, Predicate pred = Predicate())
{
    if(pred.template check<ConcreteType>(deref(param)))
    {
        handler.template on_dispatch(static_cast<typename AddQualifiers<BaseType, ConcreteType>::type>(param));
        return true;
    }
    return double_dispatch<Handler, Predicate, BaseType, ConcreteTypes...>(handler, param);
}

template<typename Handler, typename Predicate, typename BaseType>
bool double_dispatch(Handler, BaseType)
{
    return false;
}

} // namespace Impl


/**
 * Wrapper of custom_dispatch with Predicate = TypeIdPredicate
 * Calls Handler::on_dispatch(Type) for every type whose runtime type is Type
 * Rejects any type that is not explicitly specified in ConcreteTypes, even if it is a derivative of one
 * If you also want to receive types which inherit from a specified type, use relaxed_dispatch
 * See custom_dispatch for documentation of parameters
 */
template<typename Handler, typename BaseType, typename... ConcreteTypes>
bool strict_dispatch(Handler handler, BaseType param)
{
    return Impl::double_dispatch<Handler, Impl::TypeIdPredicate, BaseType, ConcreteTypes...>(handler, param);
}

/**
 * Wrapper of custom_dispatch with Predicate = DynamicCastPredicate
 * Calls Handler::on_dispatch(Type) for every type whose runtime type is Type or a derivative of Type
 * See custom_dispatch for documentation of parameters
 */
template<typename Handler, typename BaseType, typename... ConcreteTypes>
bool relaxed_dispatch(Handler handler, BaseType param)
{
    return Impl::double_dispatch<Handler, Impl::DynamicCastPredicate, BaseType, ConcreteTypes...>(handler, param);
}

/**
 * Calls Handler::on_dispatch(Type) with the correct concrete type of param
 * @tparam Handler Type to call the concrete methods on
 * @tparam Predicate Functor with bool check<ConcreteType>(BaseType) method to determine if BaseType can be casted to
 * ConcreteType
 * @tparam BaseType Base of all dispatched types. This type should be fully qualified (e.g. const, pointer / reference)
 * @tparam ConcreteTypes List of all types a concrete function is provided for. These should not be qualified
 * @param handler Object to call concrete methods on
 * @param param Base object to dispatch
 * @param pred Functor of type Predicate
 * @return True if a matching concrete function for the runtime type of param was found
 */
template<typename Handler, typename Predicate, typename BaseType, typename... ConcreteTypes>
bool custom_dispatch(Handler handler, BaseType param, Predicate pred = Predicate())
{
    return Impl::double_dispatch<Handler, Predicate, BaseType, ConcreteTypes...>(handler, param, pred);
}
