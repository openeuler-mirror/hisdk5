#ifndef NIC_MIG_MPU_INTF_H
#define NIC_MIG_MPU_INTF_H

#include "nic_cfg_comm.h"

#ifndef MAX_CEQ_PER_FUNC
#define MAX_CEQ_PER_FUNC 0x20
#endif

/**
 * @brief 定义一个枚举类型，用于表示MSIX控制寄存器的操作类型。
 * @details
 * 这个枚举类型包含两个成员，分别表示获取和设置MSIX控制寄存器的操作。
 */
enum mig_nic_msix_op {
    MSIX_CTRL_CSR_GET,      /**< 获取MSIX控制寄存器的值 */
    MSIX_CTRL_CSR_SET       /**< 设置MSIX控制寄存器的值 */
};

#define MAX_CMDQ_NUM 0x4    /**< 最大cmdq数量*/

#define MAX_SQ_NUM 0x40     /**< 最大sq数量 */

/**
 * @struct mig_nic_mac_vlan
 * @brief 定义一个结构体，用于存储网卡的MAC地址、VLAN ID和保留字段
 * @details 该结构体主要用于网络设备的配置和管理，包含了MAC地址、VLAN ID和保留字段。
 */
struct mig_nic_mac_vlan {
	u8 mac[6]; /**< MAC地址，长度为6字节 */
	u16 vlan_id; /**< VLAN ID，用于标识网络中的不同网络 */
	u16 rsvd; /**< 保留字段，当前未使用 */
};

#define MIG_FAST_MSG_MAX_PAGE_SIZE (256 * 1024)
#endif