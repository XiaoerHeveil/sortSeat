#include "sorting.h"

/**
 * @brief 交换一号位与二号位的座位号
 * 
 * @param seatSheet 座位表
 * @param oneRow 一号行
 * @param oneColumn 一号列
 * @param twoRow 二号行
 * @param twoColumn 二号列
 */
void exchageSeatNumber(int *seatSheet
    , const int &oneRow, const int &oneColumn
    , const int &twoRow, const int &twoColumn) {
    // 临时存储一号位的座位号
    int temp = seatSheet[oneRow, oneColumn];
    // 将二号位的座位号填充至一号位
    seatSheet[oneRow, oneColumn] = seatSheet[twoRow, twoColumn];
    // 将原本一号位的座位号填充至二号位
    seatSheet[twoRow, twoColumn] = temp;
}