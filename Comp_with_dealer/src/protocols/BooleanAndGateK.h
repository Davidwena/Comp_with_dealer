// By Kaiwen Wang
/// @file Generalized k-input Boolean AND gate using TinyOT
/// Implements z = x1 ∧ x2 ∧ ... ∧ xk for any k ≥ 2

#ifndef MD_ML_BOOLEANANDGATEK_H
#define MD_ML_BOOLEANANDGATEK_H

#include <memory>
#include <vector>
#include <thread>
#include <cmath>
#include <bitset>
#include <algorithm>
#include <unordered_map>

#include "protocols/GateTinyOT.h"
#include "share/IsTinyOTShare.h"

namespace md_ml {

/**
 * Generalized k-input Boolean AND gate using TinyOT authenticated shares.
 * Computes z = x1 ∧ x2 ∧ ... ∧ xk for arbitrary k ≥ 2.
 *
 * 份额形式：<xi> = (δxi, [[σi]]) for i = 1, ..., k
 *   - δxi: 打开的掩码值 δxi = xi ⊕ σxi
 *   - [[σi]]: 掩码的认证份额 = ([σi], [MAC·σi])
 *
 * 预处理：
 *   - 基础随机值：a1, ..., ak
 *   - 所有子集的AND组合：对于每个非空子集S ⊆ {1,...,k}，生成 [[∧_{i∈S} ai]]
 *   - 输出掩码：[[σz]]
 *   - 预计算并打开：εi = ai ⊕ σi for i = 1,...,k
 *
 * 在线计算：
 *   1. 对所有i：计算 mi = δxi ⊕ εi = ai ⊕ xi
 *   2. 计算份额：[[δz]] = Σ_{S⊆{1,...,k}} [ (∧_{i∉S} mi) · [[∧_{i∈S} ai]] ]
 *   3. 打开 [[δz]] 得到 δz（单次通信）
 *   4. 输出 <z> = (δz, [[σz]])
 *
 * 复杂度：
 *   - 预处理存储：O(2^k)
 *   - 在线计算：O(2^k)
 *   - 通信轮次：1
 */
template <IsTinyOTShare ShrType>
class BooleanAndGateK : public GateTinyOT<ShrType> {
public:
    using ClearType = typename ShrType::ClearType;     // bool
    using SemiShrType = typename ShrType::SemiShrType; // bool
    using MacType = typename ShrType::MacType;         // F_{2^S}

    /**
     * 构造函数：k输入AND门
     * @param p_inputs 输入门的向量，长度为k
     */
    BooleanAndGateK(const std::vector<std::shared_ptr<GateTinyOT<ShrType>>>& p_inputs);

    std::size_t k() const { return k_; }
    std::size_t batch_size() const { return batch_size_; }

private:
    void doReadOfflineFromFile() override;
    void doRunOnline() override;

    std::size_t k_;  // 输入数量
    std::size_t batch_size_;  // 批量大小

    // 预处理数据存储
    // 对于每个非空子集S，存储对应的AND结果（按mask顺序）
    std::vector<std::vector<SemiShrType>> subset_and_shares_;  // [subset_id][element_id]
    std::vector<std::vector<MacType>> subset_and_macs_;        // [subset_id][element_id]

    // 预计算的ε值
    std::vector<std::vector<ClearType>> epsilon_open_;  // [input_id][element_id]

    // 子集索引映射：子集位掩码 -> 存储索引
    std::unordered_map<std::size_t, std::size_t> subset_mask_to_index_;
};


template <IsTinyOTShare ShrType>
BooleanAndGateK<ShrType>::BooleanAndGateK(
    const std::vector<std::shared_ptr<GateTinyOT<ShrType>>>& p_inputs)
    : GateTinyOT<ShrType>(p_inputs),
      k_(p_inputs.size()){

    if (k_ < 2) {
        throw std::runtime_error("BooleanAndGateK: k must be at least 2");
    }

    // 假设所有输入门有相同的维度
    this->set_dim_row(p_inputs[0]->dim_row());
    this->set_dim_col(p_inputs[0]->dim_col());
    batch_size_ = this->dim_row() * this->dim_col();

    // 生成所有子集映射（非空子集）
    std::size_t subset_count = (1 << k_) - 1;
    subset_and_shares_.resize(subset_count);
    subset_and_macs_.resize(subset_count);

    std::size_t idx = 0;
    for (std::size_t mask = 1; mask < (1 << k_); ++mask) {
        subset_mask_to_index_[mask] = idx++;
    }
}

template <IsTinyOTShare ShrType>
void BooleanAndGateK<ShrType>::doReadOfflineFromFile() {
    // std::size_t elem_per_gate = this->dim_row() * this->dim_col();
    // std::size_t total_size = batch_size_ * elem_per_gate;

    // 1. 读取所有子集的AND结果（按mask从1到2^k-1的顺序）
    for (std::size_t mask = 1; mask < (1 << k_); ++mask) {
        auto it = subset_mask_to_index_.find(mask);
        if (it != subset_mask_to_index_.end()) {
            std::size_t idx = it->second;

            // 读取份额和MAC
            subset_and_shares_[idx] = this->party().ReadBoolShares(batch_size_);
            subset_and_macs_[idx] = this->party().ReadMacShares(batch_size_);
        }
    }

    // 2. 读取输出掩码 [[σz]]
    this->lambda_shr() = this->party().ReadBoolShares(batch_size_);
    this->lambda_shr_mac() = this->party().ReadMacShares(batch_size_);

    // 3. 读取预计算的ε值
    epsilon_open_.resize(k_);
    for (std::size_t i = 0; i < k_; ++i) {
        epsilon_open_[i].resize(batch_size_);
        for (std::size_t j = 0; j < batch_size_; ++j) {
            int eps_val;
            this->party().input_file() >> eps_val;
            epsilon_open_[i][j] = (eps_val != 0);
        }
    }
}


template <IsTinyOTShare ShrType>
void BooleanAndGateK<ShrType>::doRunOnline() {
    // 使用 this->inputs() 而不是 inputs_
    const auto& inputs = this->inputs();
    
    // 计算所有mi = δxi ⊕ εi = ai ⊕ xi
    std::vector<std::vector<ClearType>> m_values(k_,
        std::vector<ClearType>(batch_size_));
    
    for (std::size_t i = 0; i < k_; ++i) {
        const auto& delta_x = inputs[i]->Delta_clear();

        for (std::size_t j = 0; j < batch_size_; ++j) {
            // 计算在当前输入门中的位置
            // std::size_t pos_in_input = j % inputs[i]->dim_row();
            // 修正：确保不越界
            std::size_t pos_in_input = std::min(j, delta_x.size() - 1);
            
            m_values[i][j] = delta_x[pos_in_input] ^ epsilon_open_[i][j];
        }
    }
    
    // 计算 [[δz]]
    std::vector<SemiShrType> delta_z_shr(batch_size_);
    std::vector<MacType> delta_z_shr_mac(batch_size_);

    // 初始化为0
    std::fill(delta_z_shr.begin(), delta_z_shr.end(), false);
    std::fill(delta_z_shr_mac.begin(), delta_z_shr_mac.end(), MacType::zero());

    // 遍历所有子集（包括空集）
    for (std::size_t mask = 0; mask < (1 << k_); ++mask) {
        // 计算补集（不在子集中的索引）
        std::vector<std::size_t> complement_set;
        std::vector<std::size_t> subset;

        for (std::size_t i = 0; i < k_; ++i) {
            if (mask & (1 << i)) {
                subset.push_back(i);
            } else {
                complement_set.push_back(i);
            }
        }
        // 计算 ∏_{i∉S} mi
        std::vector<ClearType> m_product(batch_size_);
        if (complement_set.empty()) {
            // 空集：乘积为1
            std::fill(m_product.begin(), m_product.end(), true);
        } else {
            // 计算补集中所有mi的AND
            std::fill(m_product.begin(), m_product.end(), true);
            for (auto idx : complement_set) {
                for (std::size_t j = 0; j < batch_size_; ++j) {
                    m_product[j] = m_product[j] && m_values[idx][j];
                }
            }
        }

        // 如果子集非空，加上对应的份额
        if (!subset.empty()) {
            auto it = subset_mask_to_index_.find(mask);
            if (it != subset_mask_to_index_.end()) {
                std::size_t idx = it->second;
                const auto& shr = subset_and_shares_[idx];
                const auto& mac = subset_and_macs_[idx];

                for (std::size_t j = 0; j < batch_size_; ++j) {
                    if (m_product[j]) {
                        delta_z_shr[j] = delta_z_shr[j] ^ shr[j];
                        delta_z_shr_mac[j] = delta_z_shr_mac[j] + mac[j];
                    }
                }
            }
        } else {
            // 空集：纯明文项，只有party 0添加
            if (this->my_id() == 0) {
                for (std::size_t j = 0; j < batch_size_; ++j) {
                    if (m_product[j]) {
                        delta_z_shr[j] = true;
                    }
                }
            }
        }
    }

    // 添加输出掩码 [[σz]]
    for (std::size_t j = 0; j < batch_size_; ++j) {
        delta_z_shr[j] = delta_z_shr[j] ^ this->lambda_shr()[j];
        delta_z_shr_mac[j] = delta_z_shr_mac[j] + this->lambda_shr_mac()[j];
    }

    // 打开 [[δz]]
    std::size_t num_bytes = (batch_size_ + 7) / 8;
    std::vector<uint8_t> send_msg(num_bytes, 0);
    std::vector<uint8_t> recv_msg;

    // 打包 delta_z 份额
    for (std::size_t i = 0; i < batch_size_; ++i) {
        if (delta_z_shr[i]) {
            std::size_t byte_idx = i / 8;
            std::size_t bit_idx = i % 8;
            send_msg[byte_idx] |= (1 << bit_idx);
        }
    }

    // 并行发送和接收
    std::thread t1([this, &send_msg]() {
        this->party().SendVecToOther(send_msg);
    });
    std::thread t2([this, &recv_msg, num_bytes]() {
        recv_msg = this->party().template ReceiveVecFromOther<uint8_t>(num_bytes);
    });
    t1.join();
    t2.join();

    // 重构 δz
    this->Delta_clear().resize(batch_size_);
    for (std::size_t i = 0; i < batch_size_; ++i) {
        std::size_t byte_idx = i / 8;
        std::size_t bit_idx = i % 8;

        bool dz_self = (send_msg[byte_idx] >> bit_idx) & 1;
        bool dz_other = (recv_msg[byte_idx] >> bit_idx) & 1;
        this->Delta_clear()[i] = dz_self ^ dz_other;
    }

}


} // namespace md_ml

#endif // MD_ML_BOOLEANANDGATEK_H