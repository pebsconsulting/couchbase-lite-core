//
// DBQuery_native.cs
//
// Author:
// 	Jim Borden  <jim.borden@couchbase.com>
//
// Copyright (c) 2016 Couchbase, Inc All rights reserved.
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
    public unsafe static partial class Native
    {
        public static C4Query* c4query_new(C4Database* database, string expression, C4Error* error)
        {
            using(var expression_ = new C4String(expression)) {
                return NativeRaw.c4query_new(database, expression_.AsC4Slice(), error);
            }
        }

        [DllImport(Constants.DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void c4query_free(C4Query* x);

        public static C4QueryEnumerator* c4query_run(C4Query* query, C4QueryOptions* options, string encodedParameters, C4Error* outError)
        {
            using(var encodedParameters_ = new C4String(encodedParameters)) {
                return NativeRaw.c4query_run(query, options, encodedParameters_.AsC4Slice(), outError);
            }
        }

        public static string c4query_fullTextMatched(C4Query* query, string docID, ulong seq, C4Error* outError)
        {
            using(var docID_ = new C4String(docID)) {
                using(var retVal = NativeRaw.c4query_fullTextMatched(query, docID_.AsC4Slice(), seq, outError)) {
                    return ((C4Slice)retVal).CreateString();
                }
            }
        }

        public static bool c4db_createIndex(C4Database* database, string propertyPath, C4IndexType indexType, C4IndexOptions* indexOptions, C4Error* outError)
        {
            using(var propertyPath_ = new C4String(propertyPath)) {
                return NativeRaw.c4db_createIndex(database, propertyPath_.AsC4Slice(), indexType, indexOptions, outError);
            }
        }

        public static bool c4db_deleteIndex(C4Database* database, string propertyPath, C4IndexType indexType, C4Error* outError)
        {
            using(var propertyPath_ = new C4String(propertyPath)) {
                return NativeRaw.c4db_deleteIndex(database, propertyPath_.AsC4Slice(), indexType, outError);
            }
        }


    }
    
    public unsafe static partial class NativeRaw
    {
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern C4Query* c4query_new(C4Database* database, C4Slice expression, C4Error* error);

        [DllImport(Constants.DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern C4QueryEnumerator* c4query_run(C4Query* query, C4QueryOptions* options, C4Slice encodedParameters, C4Error* outError);

        [DllImport(Constants.DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern C4SliceResult c4query_fullTextMatched(C4Query* query, C4Slice docID, ulong seq, C4Error* outError);

        [DllImport(Constants.DllName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool c4db_createIndex(C4Database* database, C4Slice propertyPath, C4IndexType indexType, C4IndexOptions* indexOptions, C4Error* outError);

        [DllImport(Constants.DllName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool c4db_deleteIndex(C4Database* database, C4Slice propertyPath, C4IndexType indexType, C4Error* outError);


    }
}
