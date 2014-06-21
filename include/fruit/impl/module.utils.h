/*
 * Copyright 2014 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FRUIT_MODULE_UTILS_H
#define FRUIT_MODULE_UTILS_H

#include "injection_errors.h"
#include "assisted_utils.h"
#include "../fruit_forward_decls.h"
#include "fruit_assert.h"

namespace fruit {

namespace impl {

template <typename Signature>
struct IsValidSignature : std::false_type {};

template <typename T, typename... Args>
struct IsValidSignature<T(Args...)> : public static_and<!is_list<T>::value, !is_list<Args>::value...> {};

template <typename Deps>
struct IsValidDeps : std::false_type {};

template <typename... D>
struct IsValidDeps<List<D...>> : public static_and<IsValidSignature<D>::value...> {};

// Exposes a bool `value' (whether C is injectable with annotation)
template <typename C>
struct HasInjectAnnotation {
    typedef char yes[1];
    typedef char no[2];

    template <typename C1>
    static yes& test(typename C1::Inject*);

    template <typename>
    static no& test(...);
    
    static const bool value = sizeof(test<C>(0)) == sizeof(yes);
};

template <typename T, typename Signature>
struct IsConstructorSignature : std::false_type {
};

template <typename C, typename... Args>
struct IsConstructorSignature<C, C(Args...)> : std::true_type {
};

template <typename C>
struct GetInjectAnnotation {
    using S = typename C::Inject;
    using A = SignatureArgs<S>;
    static_assert(IsValidSignature<S>::value, "The Inject typedef is not of the form C(Args...)"); // Tested
    static_assert(std::is_same<C, SignatureType<S>>::value, "The Inject typedef is not of the form C(Args...). Maybe C inherited an Inject annotation from the base class by mistake?");
    static_assert(is_constructible_with_list<C, UnlabelAssisted<A>>::value, "C contains an Inject annotation but it's not constructible with the specified types"); // Tested
    static constexpr bool ok = true
        && IsValidSignature<S>::value
        && std::is_same<C, SignatureType<S>>::value
        && is_constructible_with_list<C, UnlabelAssisted<A>>::value;
    // Don't even provide them if the asserts above failed. Otherwise the compiler goes ahead and may go into a long loop,
    // e.g. with an Inject=int(C) in a class C.
    using Signature = typename std::enable_if<ok, S>::type;
    using Args = typename std::enable_if<ok, A>::type;
};

template <typename Dep>
struct DepRequirementsImpl {
};

template <typename Dep>
using DepRequirements = typename DepRequirementsImpl<Dep>::type;

template <typename C, typename Dep>
using RemoveRequirementFromDep = ConstructSignature<SignatureType<Dep>, remove_from_list<C, SignatureArgs<Dep>>>;

template <typename C, typename Deps>
struct RemoveRequirementFromDepsImpl {};

template <typename C, typename... Dep>
struct RemoveRequirementFromDepsImpl<C, List<Dep...>> {
  using type = List<RemoveRequirementFromDep<C, Dep>...>;
};

template <typename C, typename Deps>
struct RemoveRequirementFromDepsHelper {
  static_assert(false && sizeof(C*), "");
};

template <typename C, typename... Deps>
struct RemoveRequirementFromDepsHelper<C, List<Deps...>> {
  using type = List<RemoveRequirementFromDep<C, Deps>...>;
};

template <typename C, typename Deps>
using RemoveRequirementFromDeps = typename RemoveRequirementFromDepsHelper<C, Deps>::type;

template <typename P, typename Rs>
using ConstructDep = ConstructSignature<P*, list_to_set<AddPointerToList<Rs>>>;

template <typename Rs, typename... P>
using ConstructDeps = List<ConstructDep<P, Rs>...>;

// In Dep, replaces all requirements for R with the types in Rs
template <typename Dep, typename R, typename Rs>
using ReplaceRequirementsInDep = SignatureType<Dep>(replace_with_set<R, Rs, SignatureArgs<Dep>>);

template <typename Dep>
struct HasSelfLoop : is_in_list<SignatureType<Dep>, SignatureArgs<Dep>> {
};

template <typename D, typename D1>
using CanonicalizeDepWithDep = ConstructSignature<SignatureType<D>, replace_with_set<SignatureType<D1>, SignatureArgs<D1>, SignatureArgs<D>>>;

template <typename Deps, typename Dep>
struct CanonicalizeDepsWithDep {}; // Not used.

template <typename... Deps, typename Dep>
struct CanonicalizeDepsWithDep<List<Deps...>, Dep> {
  using type = List<CanonicalizeDepWithDep<Deps, Dep>...>;
};

template <typename Dep, typename Deps>
struct CanonicalizeDepWithDeps {}; // Not used.

template <typename Dep>
struct CanonicalizeDepWithDeps<Dep, List<>> {
  using type = Dep;
};

template <typename Dep, typename D1, typename... Ds>
struct CanonicalizeDepWithDeps<Dep, List<D1, Ds...>> {
  using recursion_result = typename CanonicalizeDepWithDeps<Dep, List<Ds...>>::type;
  using type = CanonicalizeDepWithDep<recursion_result, D1>;
};

template <typename Dep, typename Deps>
struct AddDepHelper {
  using CanonicalizedDep = typename CanonicalizeDepWithDeps<Dep, Deps>::type;
  FruitDelegateCheck(CheckHasNoSelfLoop<!HasSelfLoop<Dep>::value, SignatureType<Dep>>);
  // At this point CanonicalizedDep doesn't have as arguments any types appearing as heads in Deps,
  // but the head of CanonicalizedDep might appear as argument of some Deps.
  // A single replacement step is sufficient.
  using type = add_to_list<CanonicalizedDep, typename CanonicalizeDepsWithDep<Deps, CanonicalizedDep>::type>;
};

template <typename Dep, typename Deps>
using AddDep = typename AddDepHelper<Dep, Deps>::type;

template <typename Deps, typename OtherDeps>
struct AddDepsHelper {};

template <typename... OtherDeps>
struct AddDepsHelper<List<>, List<OtherDeps...>> {
  using type = List<OtherDeps...>;
};

template <typename Dep, typename... Deps, typename... OtherDeps>
struct AddDepsHelper<List<Dep, Deps...>, List<OtherDeps...>> {
  using recursion_result = typename AddDepsHelper<List<Deps...>, List<OtherDeps...>>::type;
  using type = AddDep<Dep, recursion_result>;
};

template <typename Deps, typename OtherDeps>
using AddDeps = typename AddDepsHelper<Deps, OtherDeps>::type;

template <typename D, typename Deps>
struct CheckDepEntailed {
  static_assert(false && sizeof(D), "bug! should never instantiate this.");
};

template <typename D>
struct CheckDepEntailed<D, List<>> {
  static_assert(false && sizeof(D), "The dep D has no match in Deps");
};

// DType is not D1Type, not the dep that we're looking for.
template <typename DType, typename... DArgs, typename D1Type, typename... D1Args, typename... Ds>
struct CheckDepEntailed<DType(DArgs...), List<D1Type(D1Args...), Ds...>> : public CheckDepEntailed<DType(DArgs...), List<Ds...>> {};

// Found the dep that we're looking for, check that the args are a subset.
template <typename DType, typename... DArgs, typename... D1Args, typename... Ds>
struct CheckDepEntailed<DType(DArgs...), List<DType(D1Args...), Ds...>> {
  static_assert(is_empty_list<set_difference<List<D1Args...>, List<DArgs...>>>::value, "Error, the args in the new dep are not a superset of the ones in the old one");
};

// General case: DepsSubset is empty.
template <typename DepsSubset, typename Deps>
struct CheckDepsSubset {
  static_assert(is_empty_list<DepsSubset>::value, "");
};

template <typename D1, typename... D, typename Deps>
struct CheckDepsSubset<List<D1, D...>, Deps> : CheckDepsSubset<List<D...>, Deps> {
  FruitDelegateCheck(CheckDepEntailed<D1, Deps>);
};

// General case: DepsSubset is empty.
template <typename M, typename EntailedM>
struct CheckModuleEntails {
  using AdditionalProvidedTypes = set_difference<typename EntailedM::Ps, typename M::Ps>;
  FruitDelegateCheck(CheckNoAdditionalProvidedTypes<AdditionalProvidedTypes>);
  using NoLongerRequiredTypes = set_difference<typename M::Rs, typename EntailedM::Rs>;
  FruitDelegateCheck(CheckNoTypesNoLongerRequired<NoLongerRequiredTypes>);
  FruitDelegateCheck(CheckDepsSubset<typename EntailedM::Deps, typename M::Deps>);
};

template <typename L>
struct ExpandInjectorsInParamsHelper {};

template <>
struct ExpandInjectorsInParamsHelper<List<>> {
  using type = List<>;
};

// Non-empty list, T is not of the for Injector<Ts...>
template <typename T, typename... OtherTs>
struct ExpandInjectorsInParamsHelper<List<T, OtherTs...>> {
  using recursion_result = typename ExpandInjectorsInParamsHelper<List<OtherTs...>>::type;
  using type = add_to_list<T, recursion_result>;
};

// Non-empty list, type of the form Injector<Ts...>
template <typename... Ts, typename... OtherTs>
struct ExpandInjectorsInParamsHelper<List<fruit::Injector<Ts...>, OtherTs...>> {
  using recursion_result = typename ExpandInjectorsInParamsHelper<List<OtherTs...>>::type;
  using type = concat_lists<List<Ts...>, recursion_result>;
};

template <typename L>
using ExpandInjectorsInParams = typename ExpandInjectorsInParamsHelper<L>::type;

} // namespace impl
} // namespace fruit


#endif // FRUIT_MODULE_UTILS_H
