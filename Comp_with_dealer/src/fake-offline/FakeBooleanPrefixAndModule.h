// By Kaiwen Wang

#ifndef FAKEBOOLEANPREFIXANDMODULE_H
#define FAKEBOOLEANPREFIXANDMODULE_H

#include <memory>
#include <vector>
#include <cmath>

#include "fake-offline/FakeBooleanAndGateK.h"
#include "share/IsTinyOTShare.h"
#include "fake-offline/FakeGateTinyOT.h"
// #include "fake-offline/FakeCircuitTinyOT.h"

/**
 * PrefixAND 门：计算前缀AND
 * 输入：a1, a2, ..., an
 * 输出：b1=a1, b2=a1∧a2, b3=a1∧a2∧a3, ..., bn=a1∧...∧an
 * 
 * 实现：使用树状结构组合多个AND-k门
 * 注意：这不是一个基础门，而是一个电路模块
 */

namespace md_ml {

template <IsTinyOTShare ShrType, std::size_t N>
class FakeCircuitTinyOT;

//一个电路模块存在多个输出导线
template <IsTinyOTShare ShrType, std::size_t N>
class FakeBooleanPrefixAndModule {
public:

    /**
     * 构造函数
     * @param p_inputs 输入门向量（一维为长度 二维为宽度）
     * @param max_branch 最大扇入数（树的分支因子，如4）
     * @param fake_party FakeParty引用
     */
    explicit FakeBooleanPrefixAndModule(const std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>>& p_inputs, 
                                        std::size_t max_branch, FakePartyTinyOT<ShrType, N>& fake_party);
    
    std::shared_ptr<FakeGateTinyOT<ShrType, N>> getOutput(std::size_t i) const;
    [[nodiscard]] const std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>>& outputs() const {return current_x_;}
    std::size_t size() const { return n_; }
    // std::size_t getbatch_size_() { return batch_size_; }
    std::size_t num_rounds() const { return num_rounds_; }
    void runOffline() {  // 公共方法，不是 override
        circuit_.runOffline();
    }


private:
    void buildPrefixTree();

    FakeCircuitTinyOT<ShrType, N> circuit_;
    std::size_t max_branch_ = 2;   //表示最大分支数
    // std::size_t batch_size_;  // 批量大小 = 宽度
    std::size_t n_; // p_inputs.size() = 长度
    std::size_t num_rounds_;  // 轮数 = ⌈log_k n⌉
    std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>> inputs_;
    std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>> current_x_; //用于其他门获取输出导线
};

template <IsTinyOTShare ShrType, std::size_t N>
FakeBooleanPrefixAndModule<ShrType, N>::
FakeBooleanPrefixAndModule(const std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>>& p_inputs, 
    std::size_t max_branch, FakePartyTinyOT<ShrType, N>& fake_party) 
    : circuit_(fake_party), max_branch_(max_branch), inputs_(p_inputs), n_(p_inputs.size()) {
    if (n_ < 2) {
        throw ::std::runtime_error("PrefixAND requires at least 2 inputs");
    }

    if (max_branch_ < 2) {
        throw ::std::runtime_error("max_branch must be at least 2");
    }
    // 计算轮数: ⌈log_k n⌉
    num_rounds_ = static_cast<::std::size_t>(::std::ceil(::std::log(n_) / ::std::log(max_branch_)));
    //输出长度和输入长度一致
    current_x_ = inputs_;
    // batch_size_ = inputs_[0]->dim_row() * inputs_[0]->dim_col();

    // 构建前缀树
    buildPrefixTree();

    // 构造完成后，自动将所有最终输出添加为 endpoint
    for (const auto& output : current_x_) {
        circuit_.addEndpoint(output);
    }
}

template <IsTinyOTShare ShrType, ::std::size_t N>
void FakeBooleanPrefixAndModule<ShrType, N>::buildPrefixTree() {
    for (::std::size_t j = 0; j < num_rounds_; ++j) {
        // 第 j 轮，间隔为 k^j
        std::size_t interval = static_cast<std::size_t>(std::pow(max_branch_, j));
        std::size_t group_size = static_cast<std::size_t>(std::pow(max_branch_, j + 1));
        // 创建新的结果向量
        std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>> next_x = current_x_;
        // 对每个位置 i 进行处理
        for (std::size_t i = 0; i < n_; ++i) {
            // 计算 m: 表示 i 在第 j 轮属于 ANDm 门
            std::size_t m = ((i % group_size) / interval) + 1;
            if (m > 1) {  // 只有 m > 1 时才需要创建 AND 门
                // 收集输入: <x_{i-k^j-1}>, <x_{2·k^j-1}>, ..., <x_{(m-1)·k^j-1}>, <x_i>
                std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>> and_inputs;
                
                // for (std::size_t t = 1; t < m; ++t) {
                //     std::size_t idx = t * interval - 1;
                //     if (idx < n_) {  // 边界检查
                //         and_inputs.push_back(current_x_[idx]);
                //     }
                // }
                // and_inputs.push_back(current_x_[i]);
                // // 创建 ANDm 门
                // if (and_inputs.size() > 1) {
                //     auto and_gate = circuit_.booleanAndK(and_inputs);
                //     next_x[i] = and_gate;
                // }
                // 计算当前大组的起始位置
                std::size_t group_start = (i / group_size) * group_size;
                
                // 使用大组内的相对位置
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

template <IsTinyOTShare ShrType, std::size_t N>
std::shared_ptr<FakeGateTinyOT<ShrType, N>> 
FakeBooleanPrefixAndModule<ShrType, N>::getOutput(std::size_t i) const {
    if (i >= n_) {
        throw ::std::runtime_error("Output index out of bounds");
    }
    return current_x_[i];
}

} // namespace md_ml

#endif //FAKEBOOLEANPREFIXANDMODULE_H