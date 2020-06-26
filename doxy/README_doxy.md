<!--
# Copyright (C) 2017-2018, Decawave Limited, All Rights Reserved
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#  KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
-->

## Overview

Doxygen is a documenation generator, a tool for writing software reference documentation. The documentation is written within code, and is thusrelatively easy to keep up to date.Doxygen can cross reference documentation and code, so that the reader of a document can easily refer to theactual code. 

It builds the files in hw/drivers/dw1000.

### To run doxygen, install dependencies like flex, bison, dot.

### Building Doxygen file Using doxygen command
```
 doxygen Doxyfile-dw1000.conf

```
### Reading output

Open the output file doxy_output

In the output file, you find 2 folders, html and latex.

```
 cd html

 ls

 firefox index.html

```

