// expect-compile-error trying to get an instance of T, but it is not provided by this injector
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

#include "fruit/fruit.h"

using fruit::Module;
using fruit::Injector;

struct X {
  INJECT(X()) = default;
};

fruit::Module<X> getModule() {
  return fruit::createModule();
}

int main() {
  
  Injector<X> injector(getModule());
  // Error: trying to get an instance of T, but it is not provided by this injector
  injector.get<Injector<X>>();
  
  return 0;
}
