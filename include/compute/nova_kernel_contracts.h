/* native/include/nova_kernel_contracts.h */

#ifndef NOVA_KERNEL_CONTRACTS_H
#define NOVA_KERNEL_CONTRACTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NovaTensor NovaTensor;
typedef struct NovaIRNode NovaIRNode;

typedef struct NovaObligation NovaObligation;

NovaObligation nova_contract_matmul(const NovaTensor *A,
                                        const NovaTensor *B,
                                        const NovaTensor *Bias,
                                        NovaTensor *Out);

NovaObligation nova_contract_add(const NovaTensor *A,
                                     const NovaTensor *B, NovaTensor *Out);

NovaObligation nova_contract_conv2d(const NovaTensor *X,
                                        const NovaTensor *W,
                                        const NovaTensor *Bias,
                                        NovaTensor *Y, int64_t stride_h,
                                        int64_t stride_w, int64_t pad_h,
                                        int64_t pad_w);

NovaObligation nova_contract_relu(const NovaTensor *X, NovaTensor *Y);

NovaObligation nova_contract_softmax(const NovaTensor *X, NovaTensor *Y,
                                         int64_t axis);

NovaObligation nova_contract_transpose(const NovaTensor *X,
                                           NovaTensor *Y, const int64_t *perm,
                                           int64_t perm_len);

NovaObligation nova_contract_reshape(const NovaTensor *X, NovaTensor *Y,
                                         const int64_t *new_shape,
                                         int64_t new_rank);

NovaObligation nova_contract_from_node(const NovaIRNode *node);

#ifdef __cplusplus
}
#endif

#endif
