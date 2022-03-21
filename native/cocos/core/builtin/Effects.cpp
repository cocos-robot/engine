/****************************************************************************
 Copyright (c) 2021 Xiamen Yaji Software Co., Ltd.

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

// clang-format off
#include "core/builtin/Effects.h"
#include "base/StringUtil.h"
namespace cc {

// absolute essential effects
const GzipedString BUILTIN_EFFECTS { 
"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"
"3x6ooMwCAA=="
};
} // namespace cc
