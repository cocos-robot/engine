/****************************************************************************
 Copyright (c) 2017-2022 Xiamen Yaji Software Co., Ltd.

 http://www.cocos.com

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated engine source code (the "Software"), a limited,
 worldwide, royalty-free, non-assignable, revocable and non-exclusive license
 to use Cocos Creator solely to develop games on your target platforms. You shall
 not use Cocos Creator software for developing other software or tools that's
 used for developing games. You are not granted to publish, distribute,
 sublicense, and/or sell copies of Cocos Creator.

 The software or tools in this License Agreement are licensed, not sold.
 Xiamen Yaji Software Co., Ltd. reserves all rights not expressly granted to you.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
****************************************************************************/

#pragma once

#include <string>
#include <type_traits>
#include "base/Data.h"
#include "base/Macros.h"
#include "base/Value.h"
#include "base/std/container/unordered_map.h"
#include "base/std/container/vector.h"

namespace cc {

/**
 * @addtogroup platform
 * @{
 */

class ResizableBuffer {
public:
    ~ResizableBuffer()                = default;
    virtual void  resize(size_t size) = 0;
    virtual void *buffer() const      = 0;
};

template <typename T>
class ResizableBufferAdapter {};

template <typename CharT, typename Traits, typename Allocator>
class ResizableBufferAdapter<std::basic_string<CharT, Traits, Allocator>> : public ResizableBuffer {
    using BufferType = std::basic_string<CharT, Traits, Allocator>;
    BufferType *_buffer;

public:
    explicit ResizableBufferAdapter(BufferType *buffer) : _buffer(buffer) {}
    void resize(size_t size) override {
        _buffer->resize((size + sizeof(CharT) - 1) / sizeof(CharT));
    }
    void *buffer() const override {
        // can not invoke string::front() if it is empty
        return _buffer->empty() ? nullptr : &_buffer->front();
    }
};

template <typename T>
class ResizableBufferAdapter<ccstd::vector<T>> : public ResizableBuffer {
    using BufferType = ccstd::vector<T>;
    BufferType *_buffer;

public:
    explicit ResizableBufferAdapter(BufferType *buffer) : _buffer(buffer) {}
    void resize(size_t size) override {
        _buffer->resize((size + sizeof(T) - 1) / sizeof(T));
    }
    void *buffer() const override {
        // can not invoke vector::front() if it is empty
        return _buffer->empty() ? nullptr : &_buffer->front();
    }
};

template <>
class ResizableBufferAdapter<Data> : public ResizableBuffer {
    using BufferType = Data;
    BufferType *_buffer;

public:
    explicit ResizableBufferAdapter(BufferType *buffer) : _buffer(buffer) {}
    void resize(size_t size) override {
        auto oldSize = static_cast<size_t>(_buffer->getSize());
        if (oldSize != size) {
            // need to take buffer ownership for outer memory control
            auto *old    = _buffer->takeBuffer();
            void *buffer = realloc(old, size);
            if (buffer) {
                _buffer->fastSet(static_cast<unsigned char *>(buffer), size);
            }
        }
    }
    void *buffer() const override {
        return _buffer->getBytes();
    }
};

/** Helper class to handle file operations. */
class CC_DLL FileUtils {
public:
    /**
     *  Gets the instance of FileUtils.
     */
    static FileUtils *getInstance();

    /**
     *  Destroys the instance of FileUtils.
     */
    static void destroyInstance();

    /**
     * You can inherit from platform dependent implementation of FileUtils, such as FileUtilsAndroid,
     * and use this function to set delegate, then FileUtils will invoke delegate's implementation.
     * For example, your resources are encrypted, so you need to decrypt it after reading data from
     * resources, then you can implement all getXXX functions, and engine will invoke your own getXX
     * functions when reading data of resources.
     *
     * If you don't want to system default implementation after setting delegate, you can just pass nullptr
     * to this function.
     *
     * @warning It will delete previous delegate
     * @lua NA
     */
    static void setDelegate(FileUtils *delegate);

    /**
     *  The destructor of FileUtils.
     * @js NA
     * @lua NA
     */
    virtual ~FileUtils();

    /**
     *  Purges full path caches.
     */
    virtual void purgeCachedEntries();

    /**
     *  Gets string from a file.
     */
    virtual std::string getStringFromFile(const std::string &filename);

    /**
     *  Creates binary data from a file.
     *  @return A data object.
     */
    virtual Data getDataFromFile(const std::string &filename);

    enum class Status {
        OK                 = 0,
        NOT_EXISTS         = 1, // File not exists
        OPEN_FAILED        = 2, // Open file failed.
        READ_FAILED        = 3, // Read failed
        NOT_INITIALIZED    = 4, // FileUtils is not initializes
        TOO_LARGE          = 5, // The file is too large (great than 2^32-1)
        OBTAIN_SIZE_FAILED = 6  // Failed to obtain the file size.
    };

    /**
     *  Gets whole file contents as string from a file.
     *
     *  Unlike getStringFromFile, these getContents methods:
     *      - read file in binary mode (does not convert CRLF to LF).
     *      - does not truncate the string when '\0' is found (returned string of getContents may have '\0' in the middle.).
     *
     *  The template version of can accept cc::Data, std::basic_string and ccstd::vector.
     *
     *  @code
     *  std::string sbuf;
     *  FileUtils::getInstance()->getContents("path/to/file", &sbuf);
     *
     *  ccstd::vector<int> vbuf;
     *  FileUtils::getInstance()->getContents("path/to/file", &vbuf);
     *
     *  Data dbuf;
     *  FileUtils::getInstance()->getContents("path/to/file", &dbuf);
     *  @endcode
     *
     *  Note: if you read to ccstd::vector<T> and std::basic_string<T> where T is not 8 bit type,
     *  you may get 0 ~ sizeof(T)-1 bytes padding.
     *
     *  - To write a new buffer class works with getContents, just extend ResizableBuffer.
     *  - To write a adapter for existing class, write a specialized ResizableBufferAdapter for that class, see follow code.
     *
     *  @code
     *  namespace cc { // ResizableBufferAdapter needed in cocos2d namespace.
     *  template<>
     *  class ResizableBufferAdapter<AlreadyExistsBuffer> : public ResizableBuffer {
     *  public:
     *      ResizableBufferAdapter(AlreadyExistsBuffer* buffer)  {
     *          // your code here
     *      }
     *      virtual void resize(size_t size) override  {
     *          // your code here
     *      }
     *      virtual void* buffer() const override {
     *          // your code here
     *      }
     *  };
     *  }
     *  @endcode
     *
     *  @param[in]  filename The resource file name which contains the path.
     *  @param[out] buffer The buffer where the file contents are store to.
     *  @return Returns:
     *      - Status::OK when there is no error, the buffer is filled with the contents of file.
     *      - Status::NotExists when file not exists, the buffer will not changed.
     *      - Status::OpenFailed when cannot open file, the buffer will not changed.
     *      - Status::ReadFailed when read end up before read whole, the buffer will fill with already read bytes.
     *      - Status::NotInitialized when FileUtils is not initializes, the buffer will not changed.
     *      - Status::TooLarge when there file to be read is too large (> 2^32-1), the buffer will not changed.
     *      - Status::ObtainSizeFailed when failed to obtain the file size, the buffer will not changed.
     */
    template <
        typename T,
        typename Enable = typename std::enable_if<
            std::is_base_of<ResizableBuffer, ResizableBufferAdapter<T>>::value>::type>
    Status getContents(const std::string &filename, T *buffer) {
        ResizableBufferAdapter<T> buf(buffer);
        return getContents(filename, &buf);
    }
    virtual Status getContents(const std::string &filename, ResizableBuffer *buffer);

    /**
     *  Gets resource file data from a zip file.
     *
     *  @param[in]  filename The resource file name which contains the relative path of the zip file.
     *  @param[out] size If the file read operation succeeds, it will be the data size, otherwise 0.
     *  @return Upon success, a pointer to the data is returned, otherwise nullptr.
     *  @warning Recall: you are responsible for calling free() on any Non-nullptr pointer returned.
     */
    virtual unsigned char *getFileDataFromZip(const std::string &zipFilePath, const std::string &filename, ssize_t *size);

    /** Returns the fullpath for a given filename.

     First it will try to get a new filename from the "filenameLookup" dictionary.
     If a new filename can't be found on the dictionary, it will use the original filename.
     Then it will try to obtain the full path of the filename using the FileUtils search rules: resolutions, and search paths.
     The file search is based on the array element order of search paths and resolution directories.

     For instance:

         We set two elements("/mnt/sdcard/", "internal_dir/") to search paths vector by setSearchPaths,
         and set three elements("resources-ipadhd/", "resources-ipad/", "resources-iphonehd")
         to resolutions vector by setSearchResolutionsOrder. The "internal_dir" is relative to "Resources/".

        If we have a file named 'sprite.png', the mapping in fileLookup dictionary contains `key: sprite.png -> value: sprite.pvr.gz`.
         Firstly, it will replace 'sprite.png' with 'sprite.pvr.gz', then searching the file sprite.pvr.gz as follows:

             /mnt/sdcard/resources-ipadhd/sprite.pvr.gz      (if not found, search next)
             /mnt/sdcard/resources-ipad/sprite.pvr.gz        (if not found, search next)
             /mnt/sdcard/resources-iphonehd/sprite.pvr.gz    (if not found, search next)
             /mnt/sdcard/sprite.pvr.gz                       (if not found, search next)
             internal_dir/resources-ipadhd/sprite.pvr.gz     (if not found, search next)
             internal_dir/resources-ipad/sprite.pvr.gz       (if not found, search next)
             internal_dir/resources-iphonehd/sprite.pvr.gz   (if not found, search next)
             internal_dir/sprite.pvr.gz                      (if not found, return "sprite.png")

        If the filename contains relative path like "gamescene/uilayer/sprite.png",
        and the mapping in fileLookup dictionary contains `key: gamescene/uilayer/sprite.png -> value: gamescene/uilayer/sprite.pvr.gz`.
        The file search order will be:

             /mnt/sdcard/gamescene/uilayer/resources-ipadhd/sprite.pvr.gz      (if not found, search next)
             /mnt/sdcard/gamescene/uilayer/resources-ipad/sprite.pvr.gz        (if not found, search next)
             /mnt/sdcard/gamescene/uilayer/resources-iphonehd/sprite.pvr.gz    (if not found, search next)
             /mnt/sdcard/gamescene/uilayer/sprite.pvr.gz                       (if not found, search next)
             internal_dir/gamescene/uilayer/resources-ipadhd/sprite.pvr.gz     (if not found, search next)
             internal_dir/gamescene/uilayer/resources-ipad/sprite.pvr.gz       (if not found, search next)
             internal_dir/gamescene/uilayer/resources-iphonehd/sprite.pvr.gz   (if not found, search next)
             internal_dir/gamescene/uilayer/sprite.pvr.gz                      (if not found, return "gamescene/uilayer/sprite.png")

     If the new file can't be found on the file system, it will return the parameter filename directly.

     This method was added to simplify multiplatform support. Whether you are using cocos2d-js or any cross-compilation toolchain like StellaSDK or Apportable,
     you might need to load different resources for a given file in the different platforms.

     @since v2.1
     */
    virtual std::string fullPathForFilename(const std::string &filename) const;

    /**
     *  Gets full path from a file name and the path of the relative file.
     *  @param filename The file name.
     *  @param relativeFile The path of the relative file.
     *  @return The full path.
     *          e.g. filename: hello.png, pszRelativeFile: /User/path1/path2/hello.plist
     *               Return: /User/path1/path2/hello.pvr (If there a a key(hello.png)-value(hello.pvr) in FilenameLookup dictionary. )
     *
     */
    virtual std::string fullPathFromRelativeFile(const std::string &filename, const std::string &relativeFile);

    /**
     *  Sets the array of search paths.
     *
     *  You can use this array to modify the search path of the resources.
     *  If you want to use "themes" or search resources in the "cache", you can do it easily by adding new entries in this array.
     *
     *  @note This method could access relative path and absolute path.
     *        If the relative path was passed to the vector, FileUtils will add the default resource directory before the relative path.
     *        For instance:
     *            On Android, the default resource root path is "@assets/".
     *            If "/mnt/sdcard/" and "resources-large" were set to the search paths vector,
     *            "resources-large" will be converted to "@assets/resources-large" since it was a relative path.
     *
     *  @param searchPaths The array contains search paths.
     *  @see fullPathForFilename(const char*)
     *  @since v2.1
     *  In js:var setSearchPaths(var jsval);
     *  @lua NA
     */
    virtual void setSearchPaths(const ccstd::vector<std::string> &searchPaths);

    /**
     * Get default resource root path.
     */
    const std::string &getDefaultResourceRootPath() const;

    /**
     * Set default resource root path.
     */
    void setDefaultResourceRootPath(const std::string &path);

    /**
      * Add search path.
      *
      * @since v2.1
      */
    void addSearchPath(const std::string &path, bool front = false);

    /**
     *  Gets the array of search paths.
     *
     *  @return The array of search paths which may contain the prefix of default resource root path.
     *  @note In best practise, getter function should return the value of setter function passes in.
     *        But since we should not break the compatibility, we keep using the old logic.
     *        Therefore, If you want to get the original search paths, please call 'getOriginalSearchPaths()' instead.
     *  @see fullPathForFilename(const char*).
     *  @lua NA
     */
    virtual const ccstd::vector<std::string> &getSearchPaths() const;

    /**
     *  Gets the original search path array set by 'setSearchPaths' or 'addSearchPath'.
     *  @return The array of the original search paths
     */
    virtual const ccstd::vector<std::string> &getOriginalSearchPaths() const;

    /**
     *  Gets the writable path.
     *  @return  The path that can be write/read a file in
     */
    virtual std::string getWritablePath() const = 0;

    /**
     *  Sets writable path.
     */
    virtual void setWritablePath(const std::string &writablePath);

    /**
     *  Converts the contents of a file to a ValueMap.
     *  @param filename The filename of the file to gets content.
     *  @return ValueMap of the file contents.
     *  @note This method is used internally.
     */
    virtual ValueMap getValueMapFromFile(const std::string &filename);

    /** Converts the contents of a file to a ValueMap.
     *  This method is used internally.
     */
    virtual ValueMap getValueMapFromData(const char *filedata, int filesize);

    /**
    * write a ValueMap into a plist file
    *
    *@param dict the ValueMap want to save
    *@param fullPath The full path to the file you want to save a string
    *@return bool
    */
    virtual bool writeToFile(const ValueMap &dict, const std::string &fullPath);

    /**
     *  write a string into a file
     *
     * @param dataStr the string want to save
     * @param fullPath The full path to the file you want to save a string
     * @return bool True if write success
     */
    virtual bool writeStringToFile(const std::string &dataStr, const std::string &fullPath);

    /**
     * write Data into a file
     *
     *@param data the data want to save
     *@param fullPath The full path to the file you want to save a string
     *@return bool
     */
    virtual bool writeDataToFile(const Data &data, const std::string &fullPath);

    /**
    * write ValueMap into a plist file
    *
    *@param dict the ValueMap want to save
    *@param fullPath The full path to the file you want to save a string
    *@return bool
    */
    virtual bool writeValueMapToFile(const ValueMap &dict, const std::string &fullPath);

    /**
    * write ValueVector into a plist file
    *
    *@param vecData the ValueVector want to save
    *@param fullPath The full path to the file you want to save a string
    *@return bool
    */
    virtual bool writeValueVectorToFile(const ValueVector &vecData, const std::string &fullPath);

    /**
    * Windows fopen can't support UTF-8 filename
    * Need convert all parameters fopen and other 3rd-party libs
    *
    * @param filenameUtf8 std::string name file for conversion from utf-8
    * @return std::string ansi filename in current locale
    */
    virtual std::string getSuitableFOpen(const std::string &filenameUtf8) const;

    // Converts the contents of a file to a ValueVector.
    // This method is used internally.
    virtual ValueVector getValueVectorFromFile(const std::string &filename);

    /**
     *  Checks whether a file exists.
     *
     *  @note If a relative path was passed in, it will be inserted a default root path at the beginning.
     *  @param filename The path of the file, it could be a relative or absolute path.
     *  @return True if the file exists, false if not.
     */
    virtual bool isFileExist(const std::string &filename) const;

    /**
    *  Gets filename extension is a suffix (separated from the base filename by a dot) in lower case.
    *  Examples of filename extensions are .png, .jpeg, .exe, .dmg and .txt.
    *  @param filePath The path of the file, it could be a relative or absolute path.
    *  @return suffix for filename in lower case or empty if a dot not found.
    */
    virtual std::string getFileExtension(const std::string &filePath) const;

    /**
     *  Checks whether the path is an absolute path.
     *
     *  @note On Android, if the parameter passed in is relative to "@assets/", this method will treat it as an absolute path.
     *        Also on Blackberry, path starts with "app/native/Resources/" is treated as an absolute path.
     *
     *  @param path The path that needs to be checked.
     *  @return True if it's an absolute path, false if not.
     */
    virtual bool isAbsolutePath(const std::string &path) const;

    /**
     *  Checks whether the path is a directory.
     *
     *  @param dirPath The path of the directory, it could be a relative or an absolute path.
     *  @return True if the directory exists, false if not.
     */
    virtual bool isDirectoryExist(const std::string &dirPath) const;

    /**
     *  List all files in a directory.
     *
     *  @param dirPath The path of the directory, it could be a relative or an absolute path.
     *  @return File paths in a string vector
     */
    virtual ccstd::vector<std::string> listFiles(const std::string &dirPath) const;

    /**
     *  List all files recursively in a directory.
     *
     *  @param dirPath The path of the directory, it could be a relative or an absolute path.
     *  @return File paths in a string vector
     */
    virtual void listFilesRecursively(const std::string &dirPath, ccstd::vector<std::string> *files) const;

    /**
     *  Creates a directory.
     *
     *  @param dirPath The path of the directory, it must be an absolute path.
     *  @return True if the directory have been created successfully, false if not.
     */
    virtual bool createDirectory(const std::string &dirPath);

    /**
     *  Removes a directory.
     *
     *  @param dirPath  The full path of the directory, it must be an absolute path.
     *  @return True if the directory have been removed successfully, false if not.
     */
    virtual bool removeDirectory(const std::string &dirPath);

    /**
     *  Removes a file.
     *
     *  @param filepath The full path of the file, it must be an absolute path.
     *  @return True if the file have been removed successfully, false if not.
     */
    virtual bool removeFile(const std::string &filepath);

    /**
     *  Renames a file under the given directory.
     *
     *  @param path     The parent directory path of the file, it must be an absolute path.
     *  @param oldname  The current name of the file.
     *  @param name     The new name of the file.
     *  @return True if the file have been renamed successfully, false if not.
     */
    virtual bool renameFile(const std::string &path, const std::string &oldname, const std::string &name);

    /**
     *  Renames a file under the given directory.
     *
     *  @param oldfullpath  The current fullpath of the file. Includes path and name.
     *  @param newfullpath  The new fullpath of the file. Includes path and name.
     *  @return True if the file have been renamed successfully, false if not.
     */
    virtual bool renameFile(const std::string &oldfullpath, const std::string &newfullpath);

    /**
     *  Retrieve the file size.
     *
     *  @note If a relative path was passed in, it will be inserted a default root path at the beginning.
     *  @param filepath The path of the file, it could be a relative or absolute path.
     *  @return The file size.
     */
    virtual long getFileSize(const std::string &filepath); //NOLINT(google-runtime-int)

    /** Returns the full path cache. */
    const ccstd::unordered_map<std::string, std::string> &getFullPathCache() const { return _fullPathCache; }

    static std::string normalizePath(const std::string &path);
    static std::string getFileDir(const std::string &path);

protected:
    /**
     *  The default constructor.
     */
    FileUtils();

    /**
     *  Initializes the instance of FileUtils. It will set _searchPathArray and _searchResolutionsOrderArray to default values.
     *
     *  @note When you are porting Cocos2d-x to a new platform, you may need to take care of this method.
     *        You could assign a default value to _defaultResRootPath in the subclass of FileUtils(e.g. FileUtilsAndroid). Then invoke the FileUtils::init().
     *  @return true if succeed, otherwise it returns false.
     *
     */
    virtual bool init();

    /**
     *  Checks whether a file exists without considering search paths and resolution orders.
     *  @param filename The file (with absolute path) to look up for
     *  @return Returns true if the file found at the given absolute path, otherwise returns false
     */
    virtual bool isFileExistInternal(const std::string &filename) const = 0;

    /**
     *  Checks whether a directory exists without considering search paths and resolution orders.
     *  @param dirPath The directory (with absolute path) to look up for
     *  @return Returns true if the directory found at the given absolute path, otherwise returns false
     */
    virtual bool isDirectoryExistInternal(const std::string &dirPath) const;

    /**
     *  Gets full path for filename, resolution directory and search path.
     *
     *  @param filename The file name.
     *  @param searchPath The search path.
     *  @return The full path of the file. It will return an empty string if the full path of the file doesn't exist.
     */
    virtual std::string getPathForFilename(const std::string &filename, const std::string &searchPath) const;

    /**
     *  Gets full path for the directory and the filename.
     *
     *  @note Only iOS and Mac need to override this method since they are using
     *        `[[NSBundle mainBundle] pathForResource: ofType: inDirectory:]` to make a full path.
     *        Other platforms will use the default implementation of this method.
     *  @param directory The directory contains the file we are looking for.
     *  @param filename  The name of the file.
     *  @return The full path of the file, if the file can't be found, it will return an empty string.
     */
    virtual std::string getFullPathForDirectoryAndFilename(const std::string &directory, const std::string &filename) const;

    /**
     * The vector contains search paths.
     * The lower index of the element in this vector, the higher priority for this search path.
     */
    ccstd::vector<std::string> _searchPathArray;

    /**
     * The search paths which was set by 'setSearchPaths' / 'addSearchPath'.
     */
    ccstd::vector<std::string> _originalSearchPaths;

    /**
     *  The default root path of resources.
     *  If the default root path of resources needs to be changed, do it in the `init` method of FileUtils's subclass.
     *  For instance:
     *  On Android, the default root path of resources will be assigned with "@assets/" in FileUtilsAndroid::init().
     *  Similarly on Blackberry, we assign "app/native/Resources/" to this variable in FileUtilsBlackberry::init().
     */
    std::string _defaultResRootPath;

    /**
     *  The full path cache. When a file is found, it will be added into this cache.
     *  This variable is used for improving the performance of file search.
     */
    mutable ccstd::unordered_map<std::string, std::string> _fullPathCache;

    /**
     * Writable path.
     */
    std::string _writablePath;

    /**
     *  The singleton pointer of FileUtils.
     */
    static FileUtils *sharedFileUtils;

    /**
     *  Remove null value key (for iOS)
     */
    virtual void valueMapCompact(ValueMap &valueMap);
    virtual void valueVectorCompact(ValueVector &valueVector);
};

// end of support group
/** @} */

} // namespace cc
