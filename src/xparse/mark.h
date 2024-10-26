/**
 * *****************************************************************************
 * @file        mark.h
 * @brief       
 * @author      hsz (hszsoftware@qq.com)
 * @date        2024-10-22
 * @copyright   hszsoft
 * *****************************************************************************
 */

#ifndef __XPARSE_MARK_H__
#define __XPARSE_MARK_H__

#include <string>
#include <unordered_map>

namespace xparse {

using Attrs = std::vector<std::string>;
using MarkDatabase = std::unordered_map<std::string, Attrs>;

} // namespace xparse

#endif // __XPARSE_MARK_H__