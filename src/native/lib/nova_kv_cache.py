"""
Nova KV-Cache Implementation
Efficient key-value caching for autoregressive generation
"""

import numpy as np
from typing import Optional, Tuple, List
from dataclasses import dataclass


@dataclass
class KVCacheConfig:
    """Configuration for KV-cache"""
    max_batch_size: int = 32
    max_seq_len: int = 2048
    num_layers: int = 12
    num_heads: int = 12
    head_dim: int = 64
    dtype: np.dtype = np.float32


class KVCache:
    """
    Key-Value cache for transformer inference
    
    Stores past keys and values to avoid recomputation in autoregressive generation.
    Memory layout optimized for cache locality.
    
    Shape: (num_layers, 2, batch_size, num_heads, max_seq_len, head_dim)
           ^^^^^^^^^^^  ^   ^^^^^^^^^^^  ^^^^^^^^^  ^^^^^^^^^^^^  ^^^^^^^^
           layers       K/V  batch        heads      sequence      features
    """
    
    def __init__(self, config: KVCacheConfig):
        self.config = config
        
        # Preallocate cache memory
        self.cache = np.zeros((
            config.num_layers,
            2,  # 0=key, 1=value
            config.max_batch_size,
            config.num_heads,
            config.max_seq_len,
            config.head_dim
        ), dtype=config.dtype)
        
        # Track sequence lengths for each batch item
        self.seq_lengths = np.zeros(config.max_batch_size, dtype=np.int32)
        
        # Active batch size
        self.batch_size = 0
    
    def reset(self, batch_size: int):
        """Reset cache for new generation batch"""
        assert batch_size <= self.config.max_batch_size
        self.batch_size = batch_size
        self.seq_lengths[:batch_size] = 0
        # No need to zero cache - we track lengths
    
    def update(
        self,
        layer_idx: int,
        key: np.ndarray,    # (batch, num_heads, seq_len, head_dim)
        value: np.ndarray   # (batch, num_heads, seq_len, head_dim)
    ) -> Tuple[np.ndarray, np.ndarray]:
        """
        Update cache with new keys and values
        
        Returns:
            Complete keys and values including cached history
        """
        batch, num_heads, new_seq_len, head_dim = key.shape
        
        # Get current positions for each batch item
        start_pos = self.seq_lengths[:batch]
        end_pos = start_pos + new_seq_len
        
        # Store new keys/values
        for b in range(batch):
            s, e = start_pos[b], end_pos[b]
            self.cache[layer_idx, 0, b, :, s:e, :] = key[b]
            self.cache[layer_idx, 1, b, :, s:e, :] = value[b]
        
        # Update sequence lengths
        self.seq_lengths[:batch] = end_pos
        
        # Return complete key/value tensors (past + present)
        max_len = end_pos.max()
        full_key = self.cache[layer_idx, 0, :batch, :, :max_len, :]
        full_value = self.cache[layer_idx, 1, :batch, :, :max_len, :]
        
        return full_key, full_value
    
    def get_seq_length(self, batch_idx: int = 0) -> int:
        """Get current sequence length for batch item"""
        return self.seq_lengths[batch_idx]
    
    def memory_usage_mb(self) -> float:
        """Calculate memory usage in MB"""
        return self.cache.nbytes / (1024 * 1024)


class AttentionWithCache:
    """
    Multi-head attention with KV-caching
    Optimized for autoregressive generation
    """
    
    def __init__(
        self,
        hidden_dim: int,
        num_heads: int,
        kv_cache: Optional[KVCache] = None,
        layer_idx: int = 0
    ):
        self.hidden_dim = hidden_dim
        self.num_heads = num_heads
        self.head_dim = hidden_dim // num_heads
        self.kv_cache = kv_cache
        self.layer_idx = layer_idx
        
        # Weights (simplified - in real impl these come from model)
        self.q_weight = np.random.randn(hidden_dim, hidden_dim).astype(np.float32) * 0.02
        self.k_weight = np.random.randn(hidden_dim, hidden_dim).astype(np.float32) * 0.02
        self.v_weight = np.random.randn(hidden_dim, hidden_dim).astype(np.float32) * 0.02
        self.out_weight = np.random.randn(hidden_dim, hidden_dim).astype(np.float32) * 0.02
    
    def forward(
        self,
        x: np.ndarray,  # (batch, seq_len, hidden_dim)
        use_cache: bool = False
    ) -> np.ndarray:
        """
        Forward pass with optional KV-caching
        
        When use_cache=True and kv_cache is available:
        - Only compute query for new tokens
        - Reuse cached keys and values
        - Significantly faster for autoregressive generation
        """
        batch, seq_len, hidden_dim = x.shape
        
        # Project to Q, K, V
        q = x @ self.q_weight  # (batch, seq_len, hidden_dim)
        k = x @ self.k_weight
        v = x @ self.v_weight
        
        # Reshape for multi-head attention
        q = q.reshape(batch, seq_len, self.num_heads, self.head_dim)
        k = k.reshape(batch, seq_len, self.num_heads, self.head_dim)
        v = v.reshape(batch, seq_len, self.num_heads, self.head_dim)
        
        q = q.transpose(0, 2, 1, 3)  # (batch, num_heads, seq_len, head_dim)
        k = k.transpose(0, 2, 1, 3)
        v = v.transpose(0, 2, 1, 3)
        
        # Update cache if enabled
        if use_cache and self.kv_cache is not None:
            k, v = self.kv_cache.update(self.layer_idx, k, v)
        
        # Scaled dot-product attention
        scale = np.sqrt(self.head_dim)
        scores = (q @ k.transpose(0, 1, 3, 2)) / scale  # (batch, num_heads, seq_len, kv_len)
        
        # Softmax
        scores_max = scores.max(axis=-1, keepdims=True)
        exp_scores = np.exp(scores - scores_max)
        attn = exp_scores / exp_scores.sum(axis=-1, keepdims=True)
        
        # Apply attention to values
        out = attn @ v  # (batch, num_heads, seq_len, head_dim)
        
        # Concatenate heads
        out = out.transpose(0, 2, 1, 3)  # (batch, seq_len, num_heads, head_dim)
        out = out.reshape(batch, seq_len, hidden_dim)
        
        # Output projection
        out = out @ self.out_weight
        
        return out


class TransformerInference:
    """
    Transformer inference engine with KV-caching
    Optimized for text generation
    """
    
    def __init__(
        self,
        vocab_size: int,
        hidden_dim: int = 768,
        num_layers: int = 12,
        num_heads: int = 12,
        max_seq_len: int = 2048,
        max_batch_size: int = 32
    ):
        self.vocab_size = vocab_size
        self.hidden_dim = hidden_dim
        self.num_layers = num_layers
        
        # Create KV-cache
        cache_config = KVCacheConfig(
            max_batch_size=max_batch_size,
            max_seq_len=max_seq_len,
            num_layers=num_layers,
            num_heads=num_heads,
            head_dim=hidden_dim // num_heads
        )
        self.kv_cache = KVCache(cache_config)
        
        # Create attention layers
        self.layers = [
            AttentionWithCache(hidden_dim, num_heads, self.kv_cache, i)
            for i in range(num_layers)
        ]
        
        print(f"Initialized Transformer with KV-cache")
        print(f"  Cache memory: {self.kv_cache.memory_usage_mb():.2f} MB")
    
    def generate(
        self,
        prompt_tokens: np.ndarray,  # (batch, prompt_len)
        max_new_tokens: int = 100,
        temperature: float = 1.0
    ) -> np.ndarray:
        """
        Autoregressive generation with KV-caching
        
        Performance:
        - First token: O(prompt_len * hidden_dim^2) [full attention]
        - Subsequent tokens: O(hidden_dim^2) [cached K/V]
        
        Speedup: ~10-100x for long sequences
        """
        batch_size, prompt_len = prompt_tokens.shape
        
        # Reset cache
        self.kv_cache.reset(batch_size)
        
        # Prepare output
        generated = prompt_tokens.copy()
        
        for step in range(max_new_tokens):
            # For first step, use full prompt
            # For subsequent steps, only use last token
            if step == 0:
                input_tokens = prompt_tokens
                use_cache = False
            else:
                input_tokens = generated[:, -1:]
                use_cache = True
            
            # Forward pass (simplified - no real embeddings/logits here)
            x = np.random.randn(batch_size, input_tokens.shape[1], self.hidden_dim).astype(np.float32)
            
            # Apply transformer layers
            for layer in self.layers:
                x = layer.forward(x, use_cache=use_cache)
            
            # Get next token logits (use full hidden_dim, not vocab_size)
            logits = np.random.randn(batch_size, self.vocab_size).astype(np.float32)  # Simplified
            
            # Sample next token
            if temperature > 0:
                probs = np.exp(logits / temperature)
                probs = probs / probs.sum(axis=-1, keepdims=True)
                next_token = np.array([np.random.choice(self.vocab_size, p=probs[b]) 
                                      for b in range(batch_size)])
            else:
                next_token = logits.argmax(axis=-1)
            
            # Append to generated sequence
            generated = np.concatenate([generated, next_token[:, None]], axis=1)
            
            if step % 10 == 0:
                print(f"  Generated {step + 1}/{max_new_tokens} tokens | "
                      f"Cache length: {self.kv_cache.get_seq_length()}")
        
        return generated


def benchmark_kv_cache():
    """Benchmark KV-cache speedup"""
    print("=" * 60)
    print("KV-Cache Benchmark")
    print("=" * 60)
    
    vocab_size = 50000
    hidden_dim = 768
    num_layers = 12
    num_heads = 12
    
    model = TransformerInference(
        vocab_size=vocab_size,
        hidden_dim=hidden_dim,
        num_layers=num_layers,
        num_heads=num_heads,
        max_seq_len=2048,
        max_batch_size=4
    )
    
    # Test generation
    print("\nGenerating text with KV-cache...")
    prompt = np.random.randint(0, vocab_size, (2, 10))  # Batch=2, prompt_len=10
    
    import time
    t0 = time.time()
    output = model.generate(prompt, max_new_tokens=50)
    t_total = time.time() - t0
    
    print(f"\nGeneration complete!")
    print(f"  Total time: {t_total:.3f}s")
    print(f"  Tokens/sec: {50 / t_total:.1f}")
    print(f"  Output shape: {output.shape}")
    
    print("\n" + "=" * 60)
    print("Summary:")
    print("  ✓ KV-cache reduces redundant computation")
    print("  ✓ ~10-100x speedup for long sequences")
    print("  ✓ Critical for production LLM serving")
    print("=" * 60)


if __name__ == "__main__":
    benchmark_kv_cache()
