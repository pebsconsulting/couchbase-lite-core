//
//  native_database.cc
//  Couchbase Lite Core
//
//  Created by Jens Alfke on 9/10/15.
//  Copyright (c) 2015-2016 Couchbase. All rights reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
//  Unless required by applicable law or agreed to in writing, software distributed under the
//  License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
//  either express or implied. See the License for the specific language governing permissions
//  and limitations under the License.

#include <c4Base.h>
#include <c4.h>
#include "com_couchbase_litecore_Database.h"
#include "native_glue.hh"
#include "c4BlobStore.h"
#include "c4Document+Fleece.h"
#include "Logging.hh"

#undef DEBUG_TERMINATION // Define this to install a C++ termination handler that dumps a backtrace
#ifdef DEBUG_TERMINATION
#include <execinfo.h>   // Not available in Linux or Windows?
#include <unistd.h>
#endif

using namespace litecore;
using namespace litecore::jni;


#pragma mark - DATABASE:


static jfieldID kHandleField;
static jmethodID kLoggerLogMethod;

static inline C4Database *getDbHandle(JNIEnv *env, jobject self) {
    return (C4Database *) env->GetLongField(self, kHandleField);
}

#ifdef DEBUG_TERMINATION
static void jniTerminateHandler() {
    fprintf(stderr, "***** CBFOREST UNCAUGHT C++ EXCEPTION *****\n");
    void* addrs[50];
    int n = backtrace(addrs, 50);
    backtrace_symbols_fd(addrs, n, STDERR_FILENO);
    fprintf(stderr, "***** CBFOREST NOW ABORTING *****\n");
    abort();
}
#endif

bool litecore::jni::initDatabase(JNIEnv *env) {
#ifdef DEBUG_TERMINATION
    std::set_terminate(jniTerminateHandler);    // TODO: Take this out after debugging
#endif

    jclass dbClass = env->FindClass("com/couchbase/litecore/Database");
    if (!dbClass)
        return false;
    kHandleField = env->GetFieldID(dbClass, "_handle", "J");
    if (!kHandleField)
        return false;
    jclass loggerClass = env->FindClass("com/couchbase/litecore/Logger");
    if (!loggerClass)
        return false;
    kLoggerLogMethod = env->GetMethodID(loggerClass, "log", "(ILjava/lang/String;)V");
    if (!kLoggerLogMethod)
        return false;
    return true;
}


JNIEXPORT jlong JNICALL Java_com_couchbase_litecore_Database__1open
        (JNIEnv *env, jobject self, jstring jpath,
         jint flags, jint encryptionAlg, jbyteArray encryptionKey) {
    jstringSlice path(env, jpath);

    C4DatabaseConfig config{};
    config.flags = (C4DatabaseFlags) flags;
    config.storageEngine = kC4SQLiteStorageEngine;
    if (!getEncryptionKey(env, encryptionAlg, encryptionKey, &config.encryptionKey))
        return 0;

    C4Error error;
    C4Database *db = c4db_open(path, &config, &error);
    if (!db)
        throwError(env, error);

    return (jlong) db;
}

JNIEXPORT void JNICALL Java_com_couchbase_litecore_Database_rekey
        (JNIEnv *env, jobject self, jint encryptionAlg, jbyteArray encryptionKey) {
    C4EncryptionKey key;
    if (!getEncryptionKey(env, encryptionAlg, encryptionKey, &key))
        return;

    auto db = getDbHandle(env, self);
    C4Error error;
    if (!c4db_rekey(db, &key, &error))
        throwError(env, error);
}

JNIEXPORT jstring JNICALL Java_com_couchbase_litecore_Database_getPath
        (JNIEnv *env, jobject self) {
    auto db = getDbHandle(env, self);
    C4SliceResult slice = c4db_getPath(db);
    jstring ret = toJString(env, slice);
    c4slice_free(slice);
    return ret;
}

JNIEXPORT void JNICALL Java_com_couchbase_litecore_Database_close(JNIEnv *env, jobject self) {
    auto db = getDbHandle(env, self);
    C4Error error;
    if (!c4db_close(db, &error))
        throwError(env, error);
}

JNIEXPORT void JNICALL Java_com_couchbase_litecore_Database_delete
        (JNIEnv *env, jobject self) {
    auto db = getDbHandle(env, self);
    C4Error error;
    if (!c4db_delete(db, &error))
        throwError(env, error);
}

JNIEXPORT void JNICALL Java_com_couchbase_litecore_Database_free
        (JNIEnv *env, jobject self) {
    auto db = getDbHandle(env, self);
    env->SetLongField(self, kHandleField, 0);
    c4db_free(db);
    // Note: This is called only by the finalizer, so no further calls are possible.
}

/*
 * Class:     com_couchbase_litecore_Database
 * Method:    deleteAtPath
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL
Java_com_couchbase_litecore_Database_deleteAtPath(JNIEnv *env, jclass klass, jstring jpath,
                                                  jint jflags) {
    jstringSlice path(env, jpath);
    C4DatabaseConfig config{};
    config.flags = (C4DatabaseFlags) jflags;
    C4Error error;
    if (!c4db_deleteAtPath(path, &config, &error))
        throwError(env, error);
}

JNIEXPORT void JNICALL Java_com_couchbase_litecore_Database_compact
        (JNIEnv *env, jobject self) {
    auto db = getDbHandle(env, self);
    C4Error error;
    if (!c4db_compact(db, &error))
        throwError(env, error);
}

JNIEXPORT jlong JNICALL Java_com_couchbase_litecore_Database_getDocumentCount
        (JNIEnv *env, jobject self) {
    return c4db_getDocumentCount(getDbHandle(env, self));
}


JNIEXPORT jlong JNICALL Java_com_couchbase_litecore_Database_getLastSequence
        (JNIEnv *env, jobject self) {
    return c4db_getLastSequence(getDbHandle(env, self));
}


JNIEXPORT void JNICALL Java_com_couchbase_litecore_Database_beginTransaction
        (JNIEnv *env, jobject self) {
    C4Error error;
    if (!c4db_beginTransaction(getDbHandle(env, self), &error))
        throwError(env, error);
}


JNIEXPORT void JNICALL Java_com_couchbase_litecore_Database_endTransaction
        (JNIEnv *env, jobject self, jboolean commit) {
    C4Error error;
    if (!c4db_endTransaction(getDbHandle(env, self), commit, &error))
        throwError(env, error);
}


JNIEXPORT jboolean JNICALL Java_com_couchbase_litecore_Database_isInTransaction
        (JNIEnv *env, jobject self) {
    return c4db_isInTransaction(getDbHandle(env, self));
}


#pragma mark - LOGGING:


static jobject sLoggerRef;  // Global ref to the currently registered Logger instance

// NOTE: Log should not be used in critical (GetPrimitiveArrayCritical). It causes the memory error.
static void logCallback(C4LogLevel level, C4Slice message) {
    jobject logger = sLoggerRef;
    if (logger) {
        JNIEnv *env;
        if (gJVM->GetEnv((void **) &env, JNI_VERSION_1_2) == JNI_OK) {
            env->PushLocalFrame(1);
            jobject jmessage = toJString(env, message);
            env->CallVoidMethod(logger, kLoggerLogMethod, (jint) level, jmessage);
            env->PopLocalFrame(nullptr);
        }
    }
}


JNIEXPORT void JNICALL Java_com_couchbase_litecore_Database_setLogger
        (JNIEnv *env, jclass klass, jobject logger, jint level) {
    jobject oldLoggerRef = sLoggerRef;
    sLoggerRef = env->NewGlobalRef(logger);
    if (oldLoggerRef)
        env->DeleteGlobalRef(oldLoggerRef);
    //c4log_register((C4LogLevel)level, &logCallback);
}

#pragma mark - PURGING / EXPIRING:

JNIEXPORT void JNICALL Java_com_couchbase_litecore_Database_purgeDoc
        (JNIEnv *env, jclass clazz, jlong db, jstring jdocID) {
    jstringSlice docID(env, jdocID);
    C4Error error;
    if (!c4db_purgeDoc((C4Database *) db, docID, &error))
        throwError(env, error);
}

#pragma mark - EXPIRATION:

/*
 * Class:     com_couchbase_litecore_Database
 * Method:    expirationOfDoc
 * Signature: (JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_com_couchbase_litecore_Database_expirationOfDoc
        (JNIEnv *env, jclass clazz, jlong dbHandle, jstring jdocID) {
    jstringSlice docID(env, jdocID);
    return c4doc_getExpiration((C4Database *) dbHandle, docID);
}

/*
 * Class:     com_couchbase_litecore_Database
 * Method:    setExpiration
 * Signature: (JLjava/lang/String;J)V
 */
JNIEXPORT void JNICALL Java_com_couchbase_litecore_Database_setExpiration
        (JNIEnv *env, jclass clazz, jlong dbHandle, jstring jdocID, jlong jtimestamp) {
    jstringSlice docID(env, jdocID);
    C4Error error;
    if (!c4doc_setExpiration((C4Database *) dbHandle, docID, jtimestamp, &error))
        throwError(env, error);
}

/*
 * Class:     com_couchbase_litecore_Database
 * Method:    nextDocExpiration
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_com_couchbase_litecore_Database_nextDocExpiration
        (JNIEnv *env, jclass clazz, jlong dbHandle) {
    return c4db_nextDocExpiration((C4Database *) dbHandle);
}

JNIEXPORT jobjectArray JNICALL Java_com_couchbase_litecore_Database_purgeExpiredDocuments
        (JNIEnv *env, jclass clazz, jlong dbHandle) {
    C4Error err;
    C4ExpiryEnumerator *e = c4db_enumerateExpired((C4Database *) dbHandle, &err);
    if (!e) {
        throwError(env, err);
        return 0;
    }

    std::vector<std::string> docIDs;
    while (c4exp_next(e, &err)) {
        C4SliceResult docID = c4exp_getDocID(e);
        std::string strDocID((char *) docID.buf, docID.size);
        C4Error docErr;
        if (!c4db_purgeDoc((C4Database *) dbHandle, docID, &docErr)) {
            char msg[100];
            Debug("Unable to purge expired doc: LiteCore error %d/%d: %s",
                  docErr.domain, docErr.code, c4error_getMessageC(err, msg, sizeof(msg)));
        }
        docIDs.push_back(strDocID);
        c4slice_free(docID);
    }
    if (err.code) {
        char msg[100];
        Debug("Error enumerating expired docs: LiteCore error %d/%d (%s)",
              err.domain, err.code, c4error_getMessageC(err, msg, sizeof(msg)));
    }

    c4exp_purgeExpired(e, nullptr);    // remove the expiration markers

    jobjectArray ret = (jobjectArray) env->NewObjectArray(
            (jsize) docIDs.size(),
            env->FindClass("java/lang/String"),
            env->NewStringUTF(""));
    for (int i = 0; i < docIDs.size(); i++)
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(docIDs[i].c_str()));

    if (e)
        c4exp_free(e);

    return ret;
}

#pragma mark - DOCUMENTS:

/*
 * Class:     com_couchbase_litecore_Database
 * Method:    put1
 * Signature: (JLjava/lang/String;[BLjava/lang/String;ZZ[Ljava/lang/String;IZI)J
 */
JNIEXPORT jlong JNICALL Java_com_couchbase_litecore_Database_put1
        (JNIEnv *env, jclass klass, jlong dbHandle, jstring jdocID, jbyteArray jbody,
         jstring jdocType,
         jboolean existingRevision, jboolean allowConflict,
         jobjectArray jhistory, jint flags, jboolean save, jint maxRevTreeDepth) {
    auto db = (C4Database *) dbHandle;
    jstringSlice docID(env, jdocID), docType(env, jdocType);
    C4DocPutRequest rq;
    rq.docID = docID;
    rq.docType = docType;
    rq.existingRevision = existingRevision;
    rq.allowConflict = allowConflict;
    rq.revFlags = flags;
    rq.save = save;
    rq.maxRevTreeDepth = maxRevTreeDepth;
    C4Document *doc = nullptr;
    size_t commonAncestorIndex;
    C4Error error;
    {
        // Convert jhistory, a Java String[], to a C array of C4Slice:
        jsize n = env->GetArrayLength(jhistory);
        if (env->EnsureLocalCapacity(std::min(n + 1, MaxLocalRefsToUse)) < 0)
            return -1;
        std::vector<C4Slice> history(n);
        std::vector<jstringSlice *> historyAlloc;
        for (jsize i = 0; i < n; i++) {
            jstring js = (jstring) env->GetObjectArrayElement(jhistory, i);
            jstringSlice *item = new jstringSlice(env, js);
            if (i >= MaxLocalRefsToUse)
                item->copyAndReleaseRef();
            historyAlloc.push_back(item); // so its memory won't be freed
            history[i] = *item;
        }
        rq.history = history.data();
        rq.historyCount = history.size();

        {
            // `body` is a "critical" JNI ref. This is the fastest way to access its bytes, but
            // it's illegal to make any more JNI calls until the critical ref is released.
            // We declare it in a nested block, so it'll be released immediately. (java-core#793)
            jbyteArraySlice body(env, jbody, true);
            rq.body = body;
            doc = c4doc_put(db, &rq, &commonAncestorIndex, &error);
        }

        // release memory
        for (jsize i = 0; i < n; i++)
            delete historyAlloc.at(i);
    }

    if (!doc)
        throwError(env, error);
    return (jlong) doc;
}
/*
 * Class:     com_couchbase_litecore_Database
 * Method:    put2
 * Signature: (JLjava/lang/String;JLjava/lang/String;ZZ[Ljava/lang/String;IZI)J
 */
JNIEXPORT jlong JNICALL Java_com_couchbase_litecore_Database_put2
        (JNIEnv *env, jclass klass, jlong dbHandle, jstring jdocID, jlong jbody,
         jstring jdocType,
         jboolean existingRevision, jboolean allowConflict,
         jobjectArray jhistory, jint flags, jboolean save, jint maxRevTreeDepth){
    auto db = (C4Database *) dbHandle;
    C4Slice* pBody = (C4Slice*)jbody;
    jstringSlice docID(env, jdocID), docType(env, jdocType);
    C4DocPutRequest rq;
    rq.docID = docID;
    rq.body = *pBody;
    rq.docType = docType;
    rq.existingRevision = existingRevision;
    rq.allowConflict = allowConflict;
    rq.revFlags = flags;
    rq.save = save;
    rq.maxRevTreeDepth = maxRevTreeDepth;
    C4Document *doc = nullptr;
    size_t commonAncestorIndex;
    C4Error error;
    {
        // Convert jhistory, a Java String[], to a C array of C4Slice:
        jsize n = env->GetArrayLength(jhistory);
        if (env->EnsureLocalCapacity(std::min(n + 1, MaxLocalRefsToUse)) < 0)
            return -1;
        std::vector<C4Slice> history(n);
        std::vector<jstringSlice *> historyAlloc;
        for (jsize i = 0; i < n; i++) {
            jstring js = (jstring) env->GetObjectArrayElement(jhistory, i);
            jstringSlice *item = new jstringSlice(env, js);
            if (i >= MaxLocalRefsToUse)
                item->copyAndReleaseRef();
            historyAlloc.push_back(item); // so its memory won't be freed
            history[i] = *item;
        }
        rq.history = history.data();
        rq.historyCount = history.size();

        doc = c4doc_put(db, &rq, &commonAncestorIndex, &error);

        // release memory
        for (jsize i = 0; i < n; i++)
            delete historyAlloc.at(i);
    }

    if (!doc)
        throwError(env, error);
    return (jlong) doc;
}
JNIEXPORT void JNICALL Java_com_couchbase_litecore_Database__1rawPut
        (JNIEnv *env, jclass clazz, jlong db, jstring jstore, jstring jkey, jbyteArray jmeta,
         jbyteArray jbody) {
    jstringSlice store(env, jstore);
    jstringSlice key(env, jkey);
    jbyteArraySlice meta(env, jmeta, true); // critical
    jbyteArraySlice body(env, jbody, true); // critical
    C4Error error;
    if (!c4raw_put((C4Database *) db, store, key, meta, body, &error))
        throwError(env, error);
}

JNIEXPORT jobjectArray JNICALL Java_com_couchbase_litecore_Database__1rawGet
        (JNIEnv *env, jclass clazz, jlong db, jstring jstore, jstring jkey) {
    // obtain raw document
    jstringSlice store(env, jstore);
    jstringSlice key(env, jkey);
    C4Error error;
    C4RawDocument *doc = c4raw_get((C4Database *) db, store, key, &error);
    if (doc == nullptr) {
        throwError(env, error);
        // NOTE: throwError() is not same with throw Exception() of java.
        // Need to return, otherwise, following codes will be executed.
        return nullptr;
    }

    // create two dimension array to return meta and body byte array
    jclass elemType = env->FindClass("[B");
    jobjectArray rows = env->NewObjectArray(2, elemType, nullptr);
    if (rows != nullptr) {
        env->SetObjectArrayElement(rows, 0, toJByteArray(env, doc->meta));
        env->SetObjectArrayElement(rows, 1, toJByteArray(env, doc->body));
    }

    // release raw document
    c4raw_free(doc);
    doc = nullptr;

    return rows;
}

//////// BLOB STORE API:

/*
 * Class:     com_couchbase_litecore_Database
 * Method:    getBlobStore
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_couchbase_litecore_Database_getBlobStore(JNIEnv *env, jclass clazz, jlong db) {
    C4Error error = {};
    C4BlobStore *store = c4db_getBlobStore((C4Database *) db, &error);
    if (store == NULL)
        throwError(env, error);
    return (jlong) store;
}


////////  INDEXES:  Defined in c4Query.h

/*
 * Class:     com_couchbase_litecore_Database
 * Method:    createIndex
 * Signature: (JLjava/lang/String;ILjava/lang/String;Z)Z
 */
JNIEXPORT jboolean JNICALL Java_com_couchbase_litecore_Database_createIndex(
        JNIEnv *env, jclass clazz, jlong db, jstring jexpressionsJSON, jint indexType,
        jstring jlanguage, jboolean ignoreDiacritics) {


    jstringSlice expressionsJSON(env, jexpressionsJSON);
    jstringSlice language(env, jlanguage);
    // TODO: C4IndexOptions
    C4Error error = {};
    bool res = c4db_createIndex((C4Database *) db, (C4Slice) expressionsJSON,
                                (C4IndexType) indexType, nullptr, &error);
    if (!res)
        throwError(env, error);
    return res;
}

/*
 * Class:     com_couchbase_litecore_Database
 * Method:    deleteIndex
 * Signature: (JLjava/lang/String;I)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_couchbase_litecore_Database_deleteIndex(JNIEnv *env, jclass clazz, jlong db,
                                                 jstring jexpressionsJSON, jint indexType) {
    jstringSlice expressionsJSON(env, jexpressionsJSON);
    C4Error error = {};
    bool res = c4db_deleteIndex((C4Database *) db, (C4Slice) expressionsJSON,
                                (C4IndexType) indexType, &error);
    if (!res)
        throwError(env, error);
    return res;
}

////////  FLEECE-SPECIFIC:  Defined in c4Document+Fleece.h

/*
 * Class:     com_couchbase_litecore_Database
 * Method:    createFleeceEncoder
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_couchbase_litecore_Database_createFleeceEncoder(JNIEnv *env, jclass clazz, jlong db) {
    return (jlong) c4db_createFleeceEncoder((C4Database *) db);
}

/*
 * Class:     com_couchbase_litecore_Database
 * Method:    encodeJSON
 * Signature: (J[B)J
 */
JNIEXPORT jlong JNICALL
Java_com_couchbase_litecore_Database_encodeJSON(JNIEnv *env, jclass clazz, jlong db,
                                                jbyteArray jbody) {
    jbyteArraySlice body(env, jbody, true);
    C4Error error = {};
    C4SliceResult res = c4db_encodeJSON((C4Database *) db, (C4Slice) body, &error);
    if (error.domain != 0 && error.code != 0)
        throwError(env, error);
    C4SliceResult *sliceResult = (C4SliceResult *) ::malloc(sizeof(C4SliceResult));
    sliceResult->buf = res.buf;
    sliceResult->size = res.size;
    return (jlong) sliceResult;
}