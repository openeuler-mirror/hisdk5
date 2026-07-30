#ifndef DRV_NIC_API_H
#define DRV_NIC_API_H

#include "base_type.h"
#if !defined(__UEFI__) && !defined(__VMWARE__)
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include "hinic5_lld.h"
#endif

/**
 * @brief PF设置VF link状态
 *
 * @param hwdev device pointer to hwdev
 * @param vf_link_forced VF 强制link状态，false--Link状态跟随PF，true--link状态根据link_state值
 * @param link_state link状态，false--Link down，true--link up
 * @details PF设置该PF下所有VF link状态，PF保存VF的link状态状态，
 *     未设置场景下VF link状态默认跟随PF，用户设置后以用户设置为准
 *
 * @attention: 仅PF支持
 *
 * @return: VF link状态设置成功或者失败.
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_pf_set_vf_link_state(void *hwdev, bool vf_link_forced, bool link_state);

/**
 * @brief 添加设备mac接口
 *
 * @param hwdev device pointer to hwdev
 * @param mac_addr mac地址
 * @param vlan_id vlan id 范围[0~4095]
 * @param func_id global function index
 * @param channel channel id，mailbox发送使用的channel id
 *
 * @details 添加对应function的mac地址
 *
 * @attention: 函数内部涉及发送mailbox消息会休眠，禁止中断上下文等不允许休眠的流程中调用
 *
 * @return: 添加MAC返回成功或者失败.
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_set_mac(void *hwdev, const u8 *mac_addr, u16 vlan_id, u16 func_id, u16 channel);

#if !defined(__UEFI__) && !defined(__VMWARE__)
/**
 * @brief 根据lld_dev获取网络设备句柄netdev结构体指针
 *
 * @param lld_dev device pointer to lld_dev
 *
 * @details 根据lld_dev查找nic uld设备获取netdev
 *
 * @attention: 该接口返回不会对netdev引用计数++,使用过程中如果netdev被释放可能导致访问野指针
*
 * @return: 成功匹配到lld_dev的netdev时返回netdev结构体指针，否则返回NULL
 */
struct net_device *hinic5_get_netdev_by_lld(struct hinic5_lld_dev *lld_dev);

/**
 * @brief 注册设备私有数据
 *
 * @param dev device pointer to net_device
 * @param priv 私有数据
 *
 * @details 通过net_device注册设备私有数据
 *
 * @return: 私有数据注册成功或者失败.
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_netdev_priv_set(const struct net_device *dev, void *priv);

/**
 * @brief 获取设备私有数据
 *
 * @param dev device pointer to net_device
 *
 * @details 通过net_device注册设备私有数据
 *
 * @attention: 需要先调用hinic5_netdev_priv_set接口注册设备私有数据
 *
 * @return: 私有数据.
 *     @retval NULL 失败
 *     @retval 非NULL 成功
 */
void *hinic5_netdev_priv_get(const struct net_device *dev);

/**
 * @brief NIC驱动加载钩子函数
 *
 * @param netdev device pointer to net_device
 *
 * @details 由产品重载，可实现注册文件系统文件、修改netdev名称等功能
 *
 * @return: 钩子函数运行结果.
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_probe_extend_hook(struct net_device *netdev);

/**
 * @brief NIC驱动卸载钩子
 *
 * @param netdev device pointer to net_device
 *
 * @details 由产品重载
 *
 * @return: 钩子函数运行结果.
 *     @retval 0 成功
 *     @retval 非0 失败
 */
void hinic5_remove_extend_hook(struct net_device *netdev);

struct hinic5_nt_msg {
    void *buf_in;
    void *buf_out;
    u32 in_size;
    u32 out_size;
};

/**
 * @brief NIC驱动命令钩子函数
 *
 * @param netdev device pointer to net_device
 * @param cmd 命令字
 * @param nt_msg 命令内容
 * @param support 是否支持该命令， 产品需要根据命令字判断是否支持
 *
 * @details 由产品重载
 *
 * @return: 命令执行结果.
 *     @retval 0 成功
 *     @retval 非0 失败
 */
int hinic5_tool_cmd_extend_handle(struct net_device *netdev, u32 cmd,
            struct hinic5_nt_msg *nt_msg, bool *support);

/**
 * @brief 产品侧设置用户态qps数的接口
 *
 * @param netdev device pointer to net_device
 * @param usr_qps_num 期望的用户态qps数量
 * @details 由产品调用
 *
 * @return: 命令执行结果.
 *    @retval 0 成功
 *    @retval 非0 失败
 */
int hinic5_set_usr_qps_num(struct net_device *netdev, u16 usr_qps_num);

/**
 * @brief NIC相关跳过MAC设置函数
 *
  * @param dev device pointer to net_device
 *  @param addr MAC地址
 *
 * @details 由产品重载，可实现用户态队列开启后跳过相关设置的功能
 *
 * @return: 钩子函数运行结果.
 *     @retval 0 不跳过
 *     @retval 非0 跳过
 */
int hinic5_set_mac_addr_pre_hook(struct net_device *netdev, void *addr);

/**
 * @brief NIC相关跳过MTU设置函数
 *
 * @param netdev device pointer to net_device
 * @param new_mtu 新的mtu值
 *
 * @details 由产品重载，可实现用户态队列开启后跳过相关设置的功能
 *
 * @return: 钩子函数运行结果.
 *     @retval 0 不跳过
 *     @retval 非0 跳过
 */
int hinic5_change_mtu_pre_hook(struct net_device *netdev, int new_mtu);

/**
 * @brief NIC相关跳过MTU设置函数
 *
 * @param netdev device pointer to net_device
 * @param ring 队列深度相关参数
 *
 * @details 由产品重载，可实现用户态队列开启后跳过相关设置的功能
 *
 * @return: 钩子函数运行结果.
 *     @retval 0 不跳过
 *     @retval 非0 跳过
 */
int hinic5_set_ringparam_pre_hook(struct net_device *netdev, struct ethtool_ringparam *ring);

/**
 * @brief 产品侧设置流分叉使能group数量的接口
 *
 * @param netdev device pointer to net_device
 * @param group_num 期望的group数量
 * @details 由产品调用，group_num范围为1~8。1：关闭流分叉，其他值：开启流分叉。
 * @attention 开启流分叉时，group_num实际生效值会向上取整到2的幂次。
 *
 * @return: 命令执行结果.
 *    @retval 0 成功
 *    @retval 非0 失败
 */
int hinic5_set_flow_bifurcation_group_num(struct net_device *netdev, u8 group_num);

/**
 * @brief 产品侧查询/设置流分叉功能启用时groupId对应的间接表
 *
 * @param netdev device pointer to net_device
 * @param op_code 0 查询；1 设置
 * @param group_id 设备所使用的group id
 * @param indir 间接表
 * @param indir_length 间接表长度
 * @details 由产品调用
 *
 * @return: 命令执行结果.
 *    @retval 0 成功
 *    @retval 非0 失败
 */
int hinic5_cfg_flow_bifurcation_paras(struct net_device *netdev, u8 op_code,
            u8 group_id, u32 *indir, u16 indir_length);
#endif /* !defined(__UEFI__) && !defined(__VMWARE__) */
#endif
