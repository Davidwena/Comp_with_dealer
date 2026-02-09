// By Boshi Yuan
/// @file Fake preprocessing for generalized k-input Boolean AND gates using TinyOT

#ifndef MD_ML_FAKEBOOLEANANDGATEK_H
#define MD_ML_FAKEBOOLEANANDGATEK_H

#include <vector>
#include <memory>
#include <unordered_map>

#include "fake-offline/FakeGateTinyOT.h"
#include "share/IsTinyOTShare.h"

namespace md_ml {

/**
 * 批量k输入AND门的预处理数据生成器。
 *
 * 为 k 输入 AND 操作生成预处理数据：z = x1 ∧ x2 ∧ ... ∧ xk
 *
 * 预处理数据包括：
 *   - 基础随机值：a1, ..., ak
 *   - 所有子集的AND组合：对于每个非空子集S ⊆ {1,...,k}，生成 [[∧_{i∈S} ai]]
 *   - 输出掩码：[[σz]]
 *   - 预计算值：εi = ai ⊕ σi for i = 1,...,k
 */
template <IsTinyOTShare ShrType, std::size_t N>
class FakeBooleanAndGateK : public FakeGateTinyOT<ShrType, N> {
public:
    using ClearType = typename ShrType::ClearType;     // bool
    using SemiShrType = typename ShrType::SemiShrType; // bool
    using MacType = typename ShrType::MacType;         // F_{2^S}

    /**
     * 构造函数：k输入AND门预处理
     * @param p_inputs 输入门的向量，长度为k
     */
    FakeBooleanAndGateK(
        const std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>>& p_inputs);

    std::size_t k() const { return k_; }
    std::size_t batch_size() const { return batch_size_; }

private:
    void doRunOffline() override;

    // 生成所有子集的AND结果（一维数组，顺序对应子集mask）
    std::vector<typename FakePartyTinyOT<ShrType, N>::AuthenticatedBit>
    generateSubsetANDs(const std::vector<ClearType>& a_values);

    // 子集索引映射：子集位掩码 -> 存储索引
    std::unordered_map<std::size_t, std::size_t> subset_mask_to_index_;
    std::size_t k_;  // 输入数量
    std::size_t batch_size_;  // 批量大小

};


template <IsTinyOTShare ShrType, std::size_t N>
FakeBooleanAndGateK<ShrType, N>::FakeBooleanAndGateK(
    const std::vector<std::shared_ptr<FakeGateTinyOT<ShrType, N>>>& p_inputs)
    : FakeGateTinyOT<ShrType, N>(p_inputs),
      k_(p_inputs.size()){

    if (k_ < 2) {
        throw std::runtime_error("FakeBooleanAndGateK: k must be at least 2");
    }

    this->set_dim_row(p_inputs[0]->dim_row());
    this->set_dim_col(p_inputs[0]->dim_col());
    batch_size_ = this->dim_row() * this->dim_col();

    // 生成所有子集映射（非空子集）
    std::size_t idx = 0;
    for (std::size_t mask = 1; mask < (1 << k_); ++mask) {
        subset_mask_to_index_[mask] = idx++;
    }
}

template <IsTinyOTShare ShrType, std::size_t N>
std::vector<typename FakePartyTinyOT<ShrType, N>::AuthenticatedBit>
FakeBooleanAndGateK<ShrType, N>::generateSubsetANDs(const std::vector<ClearType>& a_values) {
    std::vector<typename FakePartyTinyOT<ShrType, N>::AuthenticatedBit> result;

    // 生成所有非空子集的AND结果
    std::size_t subset_count = (1 << k_) - 1;
    result.reserve(subset_count);

    for (std::size_t mask = 1; mask < (1 << k_); ++mask) {
        ClearType and_result = true;

        // 计算子集的AND
        for (std::size_t i = 0; i < k_; ++i) {
            if (mask & (1 << i)) {
                and_result = and_result && a_values[i];
            }
        }

        result.push_back(this->fake_party().GenerateAuthenticatedBit(and_result));
    }

    return result;
}

template <IsTinyOTShare ShrType, std::size_t N>
void FakeBooleanAndGateK<ShrType, N>::doRunOffline() {
    
    // 使用 this->inputs() 而不是 inputs_
    const auto& inputs = this->inputs();
    // 初始化输出掩码存储
    this->lambda_clear().resize(batch_size_);

    // 存储所有生成的数据
    struct PreprocessingData {
        std::vector<ClearType> a_values;  // a1, ..., ak
        std::vector<ClearType> epsilon_values;  // ε1, ..., εk
        ClearType sigma_z;  // 输出掩码

        // 所有子集的认证AND结果（按mask顺序）
        std::vector<typename FakePartyTinyOT<ShrType, N>::AuthenticatedBit> subset_and_auths;

        // 输出掩码认证
        typename FakePartyTinyOT<ShrType, N>::AuthenticatedBit sigma_z_auth;
    };

    std::vector<PreprocessingData> all_data(batch_size_);

    // 为每个元素生成预处理数据
    for (std::size_t elem_idx = 0; elem_idx < batch_size_; ++elem_idx) {
        auto& data = all_data[elem_idx];
        // 1. 生成随机值 a1, ..., ak
        data.a_values.resize(k_);
        for (std::size_t i = 0; i < k_; ++i) {
            data.a_values[i] = getRand<uint32_t>() & 1;
        }

        // 2. 生成输出掩码 σz
        data.sigma_z = getRand<uint32_t>() & 1;
        this->lambda_clear()[elem_idx] = data.sigma_z;

        // 3. 计算εi = ai ⊕ σi（从输入掩码获取）
        // 每个输入是一个向量，索引为 elem_idx % dim_row
        data.epsilon_values.resize(k_);
        for (std::size_t i = 0; i < k_; ++i) {
            // 计算在当前输入门中的位置
            // std::size_t pos_in_input = elem_idx % inputs[i]->dim_row();
            std::size_t pos_in_input = elem_idx % batch_size_;
            const auto& sigma_i = inputs[i]->lambda_clear()[pos_in_input];
            data.epsilon_values[i] = data.a_values[i] ^ sigma_i;
        }

        // 4. 生成所有子集的AND结果
        data.subset_and_auths = generateSubsetANDs(data.a_values);

        // 5. 生成输出掩码认证
        data.sigma_z_auth = this->fake_party().GenerateAuthenticatedBit(data.sigma_z);
    }

    // 写入文件（按参与方）
    for (std::size_t party_id = 0; party_id < N; ++party_id) {
        auto& file = this->fake_party().ithPartyFile(party_id);

        // 写入所有子集的AND结果（按mask从1到2^k-1的顺序）
        for (std::size_t mask = 1; mask < (1 << k_); ++mask) {
            auto it = subset_mask_to_index_.find(mask);
            if (it != subset_mask_to_index_.end()) {
                std::size_t subset_idx = it->second;

                // 写入比特份额
                for (std::size_t elem_idx = 0; elem_idx < batch_size_; ++elem_idx) {
                    bool bit_share = all_data[elem_idx].subset_and_auths[subset_idx].bit_shares[party_id];
                    file << (bit_share ? 1 : 0) << '\n';
                }

                // 写入MAC份额
                for (std::size_t elem_idx = 0; elem_idx < batch_size_; ++elem_idx) {
                    file << all_data[elem_idx].subset_and_auths[subset_idx].mac_shares[party_id] << '\n';
                }
            }
        }

        // 写入输出掩码 [[σz]]
        for (std::size_t elem_idx = 0; elem_idx < batch_size_; ++elem_idx) {
            file << (all_data[elem_idx].sigma_z_auth.bit_shares[party_id] ? 1 : 0) << '\n';
        }
        for (std::size_t elem_idx = 0; elem_idx < batch_size_; ++elem_idx) {
            file << all_data[elem_idx].sigma_z_auth.mac_shares[party_id] << '\n';
        }

        // 写入ε值（明文）
        for (std::size_t i = 0; i < k_; ++i) {
            for (std::size_t elem_idx = 0; elem_idx < batch_size_; ++elem_idx) {
                file << (all_data[elem_idx].epsilon_values[i] ? 1 : 0) << '\n';
            }
        }
    }
}


} // namespace md_ml

#endif // MD_ML_FAKEBOOLEANANDGATEK_H