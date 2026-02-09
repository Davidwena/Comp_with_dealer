// By Kaiwen Wang
/// @file PrefixAND circuit module for online phase

#ifndef MD_ML_BOOLEANPREFIXANDMODULE_H
#define MD_ML_BOOLEANPREFIXANDMODULE_H

#include <memory>
#include <vector>
#include <cmath>

// #include "protocols/CircuitTinyOT.h"
#include "protocols/BooleanAndGateK.h"
#include "protocols/GateTinyOT.h"
#include "share/IsTinyOTShare.h"

namespace md_ml {

template <IsTinyOTShare ShrType>
class CircuitTinyOT;
/**
 * PrefixAND 电路模块（在线阶段）
 * 
 * 输入：a1, a2, ..., an
 * 输出：b1=a1, b2=a1∧a2, b3=a1∧a2∧a3, ..., bn=a1∧...∧an
 * 
 * 使用对数深度树状结构组合多个AND-k门
 * 电路结构必须与 FakeBooleanPrefixAndModule 完全一致
 * 
 * 复杂度：
 *   - 电路深度：O(log_k n)
 *   - 门数量：O(n log n)
 *   - 通信轮次：O(log_k n)
 */

template <IsTinyOTShare ShrType>
class BooleanPrefixAndModule {
public:
    using ClearType = typename ShrType::ClearType;

    /**
     * 构造函数
     * @param p_inputs 输入门向量（长度为n）
     * @param max_branch 最大扇入数（如4）
     * @param party Party引用
     */
    explicit BooleanPrefixAndModule(
        const ::std::vector<::std::shared_ptr<GateTinyOT<ShrType>>>& p_inputs,
        std::size_t max_branch, PartyWithFakeOfflineTinyOT<ShrType>& party);

    /**
     * 从文件读取预处理数据
     */
    void readOfflineFromFile() {
        circuit_.readOfflineFromFile();
    }
    /**
     * 运行在线计算
     */
    void runOnline() {
        circuit_.runOnline();
    }
    /**
     * 获取第 i 个输出门
     */
    std::shared_ptr<GateTinyOT<ShrType>> getOutput(std::size_t i) const;
    /**
     * 获取所有输出门
     */
    [[nodiscard]] const std::vector<std::shared_ptr<GateTinyOT<ShrType>>>& 
    outputs() const { return current_x_; }
    [[nodiscard]] std::size_t size() const { return n_; }
    [[nodiscard]] std::size_t num_rounds() const { return num_rounds_; }
    
    

private:
    void buildPrefixTree();
    CircuitTinyOT<ShrType> circuit_;  // 内部电路管理器
    std::size_t max_branch_;        // 最大扇入数
    std::size_t n_;                 // 输入长度
    std::size_t num_rounds_;        // 轮数
    std::vector<std::shared_ptr<GateTinyOT<ShrType>>> inputs_;
    std::vector<std::shared_ptr<GateTinyOT<ShrType>>> current_x_;  // 当前层结果
};

template <IsTinyOTShare ShrType>
BooleanPrefixAndModule<ShrType>::BooleanPrefixAndModule(
    const std::vector<std::shared_ptr<GateTinyOT<ShrType>>>& p_inputs,
    std::size_t max_branch, PartyWithFakeOfflineTinyOT<ShrType>& party)
    : circuit_(party), max_branch_(max_branch), inputs_(p_inputs), n_(p_inputs.size()) {
    if (n_ < 2) {
        throw ::std::runtime_error("PrefixAND requires at least 2 inputs");
    }

    if (max_branch_ < 2) {
        throw ::std::runtime_error("max_branch must be at least 2");
    }
    // 计算轮数: ⌈log_k n⌉
    num_rounds_ = static_cast<std::size_t>(std::ceil(::std::log(n_) / ::std::log(max_branch_)));
    // 初始化输出为输入
    current_x_ = inputs_;
    //构建前缀树（必须与 FakeBooleanPrefixAndModule 完全一致）
    buildPrefixTree();
    // 构造完成后，自动将所有最终输出添加为 endpoint
    for (const auto& output : current_x_) {
        circuit_.addEndpoint(output);
    }
}

template <IsTinyOTShare ShrType>
void BooleanPrefixAndModule<ShrType>::buildPrefixTree() {
    //  与预处理阶段完全相同的逻辑
    for (std::size_t j = 0; j < num_rounds_; ++j) {
        // 第 j 轮，间隔为 k^j
        std::size_t interval = static_cast<std::size_t>(std::pow(max_branch_, j));
        std::size_t group_size = static_cast<std::size_t>(std::pow(max_branch_, j + 1));
        
        // 创建新的结果向量
        std::vector<std::shared_ptr<GateTinyOT<ShrType>>> next_x = current_x_;
        
        // 对每个位置 i 进行处理
        for (std::size_t i = 0; i < n_; ++i) {
            // 计算 m: 表示 i 在第 j 轮属于 ANDm 门
            std::size_t m = ((i % group_size) / interval) + 1;
            
            if (m > 1) {  // 只有 m > 1 时才需要创建 AND 门
                // 收集输入: <x_{i-(m-1)·k^j}>, ..., <x_{i-k^j}>, <x_i>
                std::vector<std::shared_ptr<GateTinyOT<ShrType>>> and_inputs;
                
                // for (std::size_t t = 1; t < m; ++t) {
                //     std::size_t idx = i - (m - t) * interval;
                //     if (idx < n_) {  // 边界检查
                //         and_inputs.push_back(current_x_[idx]);
                //     }
                // }
                // 使用每组最后一个元素的绝对位置
                // for (std::size_t t = 1; t < m; ++t) {
                //     std::size_t idx = t * interval - 1;  // t·k^j - 1
                //     and_inputs.push_back(current_x_[idx]);
                // }
                // and_inputs.push_back(current_x_[i]);
                
                // // 创建 ANDm 门（使用在线版本）
                // if (and_inputs.size() > 1) {
                //     auto and_gate = circuit_.booleanAndK(and_inputs);
                //     next_x[i] = and_gate;
                // }

                // ✅ 计算当前大组的起始位置
                std::size_t group_start = (i / group_size) * group_size;
                
                // ✅ 使用大组内的相对位置
                for (std::size_t t = 1; t < m; ++t) {
                    std::size_t idx = group_start + t * interval - 1;
                    and_inputs.push_back(current_x_[idx]);
                }
                and_inputs.push_back(current_x_[i]);
                
                if (and_inputs.size() > 1) {
                    auto and_gate = circuit_.booleanAndK(and_inputs);
                    next_x[i] = and_gate;
                }
            }
        }
        
        // 更新 current_x 为下一轮的输入
        current_x_ = next_x;
    }
}

template <IsTinyOTShare ShrType> std::shared_ptr<GateTinyOT<ShrType>> 
BooleanPrefixAndModule<ShrType>::getOutput(::std::size_t i) const {
    if (i >= n_) {
        throw ::std::runtime_error("Output index out of bounds");
    }
    return current_x_[i];
}
} // namespace md_ml

#endif // MD_ML_BOOLEANPREFIXANDMODULE_H