#pragma once

#include <string>

// 前端传入数据的格式校验与规范化（忽略全角/半角符号差异）
namespace validate {

// 规范化学生输入：
//   - 全角符号 ：；， → 半角 : ; ,
//   - 分号/逗号/空格 → 换行（一行多人拆成多行）
//   - 全角冒号 → 半角冒号
// 输出 "姓名:性别" 每行一条（半角冒号），空行已剔除
std::string normalizeStudentInput(const std::string &raw);

// 规范化规则输入：
//   - 全角双引号 “” → 半角 "
//   - 全角括号 （） → 半角 ()
//   - 全角逗号 ， → 半角 ,
//   - 全角分号 ； / 半角分号 ; → 换行
std::string normalizeRulesInput(const std::string &raw);

} // namespace validate
