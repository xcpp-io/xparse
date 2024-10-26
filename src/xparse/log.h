/**
 * *****************************************************************************
 * @file        log.h
 * @brief       
 * @author      hsz (hszsoftware@qq.com)
 * @date        2024-10-24
 * @copyright   hszsoft
 * *****************************************************************************
 */

#ifndef __XPARSE_LOG_H__
#define __XPARSE_LOG_H__

#include <llvm/Support/FormatVariadic.h>

#define XPARSE_LOG_FORMAT "[xparse] {0}\n"

#define XPARSE_LOG_INFO(MSG, ...) \
    llvm::outs() << llvm::formatv(XPARSE_LOG_FORMAT, llvm::formatv(MSG, __VA_ARGS__))

#define XPARSE_LOG_ERROR(MSG, ...) \
    llvm::errs() << llvm::formatv(XPARSE_LOG_FORMAT, llvm::formatv(MSG, __VA_ARGS__))

#endif