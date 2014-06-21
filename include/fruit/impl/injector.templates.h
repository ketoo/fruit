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

#ifndef FRUIT_INJECTOR_TEMPLATES_H
#define FRUIT_INJECTOR_TEMPLATES_H

namespace fruit {

template <typename... P>
Injector<P...>::Injector(fruit::impl::UnsafeModule& unsafeModule)
  : unsafeModule(&unsafeModule, fruit::impl::NopDeleter<fruit::impl::UnsafeModule>()) {
}

template <typename... P>
Injector<P...>::Injector(const M& m)
  : unsafeModule(std::make_shared<fruit::impl::UnsafeModule>(m.unsafeModule)) {
};

template <typename... P>
Injector<P...>::Injector(M&& m)
  : unsafeModule(std::make_shared<fruit::impl::UnsafeModule>(std::move(m.unsafeModule))) {
};

template <typename... P>
template <typename T>
T Injector<P...>::get() {
  static_assert(fruit::impl::is_in_list<impl::GetClassForType<T>, Ps>::value,
                "trying to get an instance of T, but it is not provided by this injector");
  return unsafeModule->template get<T>();
}

template <typename... P>
template <typename T>
Injector<P...>::operator T() {
  return get<T>();
}


} // namespace fruit


#endif // FRUIT_INJECTOR_TEMPLATES_H
