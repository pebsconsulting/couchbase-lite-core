//
// Replicator_native.cs
//
// Author:
// 	Jim Borden  <jim.borden@couchbase.com>
//
// Copyright (c) 2017 Couchbase, Inc All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

using System;
using System.Linq;
using System.Runtime.InteropServices;

using LiteCore.Util;

namespace LiteCore.Interop
{
#if LITECORE_PACKAGED
    internal
#else
    public
#endif 
    unsafe static partial class Native
    {
        public static C4Replicator* c4repl_new(C4Database* db, C4Address remoteAddress, string remoteDatabaseName, C4Database* otherLocalDB, C4ReplicatorMode push, C4ReplicatorMode pull, C4ReplicatorStatusChangedCallback onStateChanged, void* callbackContext, C4Error* err)
        {
            using(var remoteDatabaseName_ = new C4String(remoteDatabaseName)) {
                return NativeRaw.c4repl_new(db, remoteAddress, remoteDatabaseName_.AsC4Slice(), otherLocalDB, push, pull, onStateChanged, callbackContext, err);
            }
        }

        [DllImport(Constants.DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void c4repl_free(C4Replicator* repl);

        [DllImport(Constants.DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void c4repl_stop(C4Replicator* repl);

        [DllImport(Constants.DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern C4ReplicatorStatus c4repl_getStatus(C4Replicator* repl);


    }
    
#if LITECORE_PACKAGED
    internal
#else
    public
#endif 
    unsafe static partial class NativeRaw
    {
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern C4Replicator* c4repl_new(C4Database* db, C4Address remoteAddress, C4Slice remoteDatabaseName, C4Database* otherLocalDB, C4ReplicatorMode push, C4ReplicatorMode pull, C4ReplicatorStatusChangedCallback onStateChanged, void* callbackContext, C4Error* err);


    }
}
