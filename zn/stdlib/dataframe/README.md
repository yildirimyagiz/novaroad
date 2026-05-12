# Nova DataFrame

**A modern data manipulation library for Nova, inspired by Pandas**

Version: 0.1.0-nova

---

## 🎯 Overview

Nova DataFrame is a high-performance data manipulation library that brings Pandas-like functionality to Nova, leveraging Nova's powerful type system, unit algebra, and compile-time guarantees.

### Key Features

✅ **Type-Safe Data Structures**
- Series (1D labeled arrays)
- DataFrame (2D labeled tables)
- Multiple index types (RangeIndex, Int64Index)

✅ **Rich Type System**
- Nullable types with explicit null handling
- Support for numeric, string, datetime types
- Type inference and compile-time checking

✅ **I/O Operations**
- CSV read/write
- JSON read/write
- Extensible format support

✅ **Core Operations**
- GroupBy (split-apply-combine)
- Merge/Join (inner, left, right, outer, cross)
- Filtering and selection
- Aggregations (sum, mean, min, max, std)

✅ **Nova-Specific Features**
- Unit algebra for physical quantities
- Tensor integration for numeric columns
- Flow types for streaming data

---

## 📦 Installation

```nova
use dataframe::{DataFrame, Series, read_csv};
```

---

## 🚀 Quick Start

### Creating a Series

```nova
use dataframe::Series;

// From vector
let prices = Series::new(vec![100.0, 150.0, 120.0, 180.0, 200.0])
    .with_name("price".to_string());

// Statistics
println!("Mean: {}", prices.mean());    // 150.0
println!("Sum: {}", prices.sum());      // 750.0
println!("Std: {}", prices.std());      // 39.05...
```

### Creating a DataFrame

```nova
use dataframe::DataFrame;
use std::collections::HashMap;

// From column map
let mut data = HashMap::new();
data.insert("product".to_string(), vec!["Apple", "Banana", "Orange"]);
data.insert("price".to_string(), vec![100.0, 150.0, 120.0]);
data.insert("qty".to_string(), vec![50, 30, 40]);

let df = DataFrame::from_columns(data);
df.info();
// Output:
// DataFrame: 3 rows × 3 columns
// Columns:
//   product (Object) - 3 non-null
//   price (Object) - 3 non-null
//   qty (Object) - 3 non-null
```

### Reading CSV

```nova
use dataframe::read_csv;

let df = read_csv("data/sales.csv")?;
println!("Shape: {:?}", df.shape());  // (100, 5)
```

### GroupBy Operations

```nova
// Group by column and aggregate
let grouped = df.groupby(vec!["region".to_string()]);
let totals = grouped.sum();
let averages = grouped.mean();
let counts = grouped.count();
```

### Merge/Join

```nova
use dataframe::{merge, JoinType};

let result = merge(
    &customers_df,
    &orders_df,
    vec!["customer_id".to_string()],
    JoinType::Inner
);
```

---

## 📚 Core Concepts

### 1. Series (1D Array)

A Series is a one-dimensional labeled array that can hold any data type.

```nova
// Create Series
let s = Series::new(vec![1.0, 2.0, 3.0, 4.0, 5.0]);

// Access elements
let val = s.get(0);           // By position
let val = s.at(0);            // By index label

// Slice
let slice = s.slice(1, 4);    // rows 1-3

// Filter
let mask = vec![true, false, true, false, true];
let filtered = s.filter(&mask);

// Statistics (for numeric Series)
s.sum();      // Total
s.mean();     // Average
s.min();      // Minimum
s.max();      // Maximum
s.std();      // Standard deviation

// Null handling
s.has_nulls();    // Check for nulls
s.count();        // Count non-nulls
s.dropna();       // Remove nulls
```

### 2. DataFrame (2D Table)

A DataFrame is a two-dimensional labeled data structure with columns of potentially different types.

```nova
// Create
let df = DataFrame::new();
df.add_column("name".to_string(), vec!["Alice", "Bob", "Charlie"]);
df.add_column("age".to_string(), vec![25, 30, 35]);

// Shape
let (rows, cols) = df.shape();
let nrows = df.len();
let ncols = df.width();

// Column access
let names = df.get_column::<String>("name");

// Selection
let subset = df.select(&["name", "age"]);

// Info
df.info();
```

### 3. Index Types

```nova
// RangeIndex (0, 1, 2, ...)
let idx = RangeIndex::new(100);  // 0 to 99

// Custom range
let idx = RangeIndex::from_range(10, 50, 2);  // 10, 12, 14, ..., 48
```

### 4. Data Types

Nova DataFrame supports rich type system:

```nova
expose cases DType {
    Int8, Int16, Int32, Int64,
    UInt8, UInt16, UInt32, UInt64,
    Float32, Float64,
    Bool,
    String,
    DateTime,
    TimeDelta,
    Categorical,
    Object,
}
```

### 5. Nullable Values

Explicit null handling with type safety:

```nova
use dataframe::{Nullable, NullValue};

let val = Nullable::Value(42.0);
let null = Nullable::Null(NullValue::NA);

// Check
if val.is_null() { /* ... */ }

// Unwrap
let v = val.unwrap();              // Panics if null
let v = val.unwrap_or(0.0);        // Default value
```

---

## 🔧 Operations

### GroupBy (Split-Apply-Combine)

```nova
// Group by single column
let grouped = df.groupby(vec!["category".to_string()]);

// Aggregations
let sum_df = grouped.sum();
let mean_df = grouped.mean();
let count_df = grouped.count();

// Number of groups
println!("Groups: {}", grouped.ngroups());

// Custom aggregation
let custom = grouped.apply(|group_df| {
    // Custom function on each group
    group_df
});
```

### Merge/Join

```nova
use dataframe::{merge, JoinType};

// Inner join
let inner = merge(&left, &right, vec!["key".to_string()], JoinType::Inner);

// Left join
let left_join = merge(&left, &right, vec!["key".to_string()], JoinType::Left);

// Outer join
let outer = merge(&left, &right, vec!["key".to_string()], JoinType::Outer);

// Cross join (Cartesian product)
let cross = merge(&left, &right, vec![], JoinType::Cross);
```

### Concatenation

```nova
use dataframe::concat;

// Vertical stack (append rows)
let combined = concat(vec![df1, df2, df3], 0);

// Horizontal stack (append columns)
let wide = concat(vec![df1, df2], 1);
```

### Filtering

```nova
// Boolean mask on Series
let prices = series.filter(&mask);

// DataFrame selection
let subset = df.select(&["col1", "col2", "col3"]);
```

---

## 📖 I/O Operations

### CSV

```nova
use dataframe::{read_csv, to_csv, CSVReader};

// Simple read
let df = read_csv("data.csv")?;

// With options
let df = CSVReader::new("data.csv".to_string())
    .delimiter('\t')
    .skip_rows(2)
    .read()?;

// Write
df.to_csv("output.csv")?;
```

### JSON

```nova
use dataframe::{read_json, to_json};

// Read (records format)
let df = read_json("data.json")?;

// Write
df.to_json("output.json")?;
```

---

## 🎨 Advanced Features

### 1. Nova Unit Algebra Integration

```nova
// Physical quantities with units
let distances = Series::new(vec![5.0.m, 10.0.km, 100.0.cm]);
let times = Series::new(vec![2.0.s, 5.0.s, 10.0.s]);

// Compile-time unit checking
let speeds = distances / times;  // Returns m/s
```

### 2. Tensor Integration

```nova
// Numeric columns backed by tensors for efficiency
let numeric_cols = df.select(&["price", "quantity", "total"]);
// Internally uses Nova tensor operations
```

### 3. Flow Types for Streaming

```nova
// Stream DataFrames for real-time processing
let stream = Stream::from_csv("live_data.csv");
stream.map(|row_df| {
    // Process each row as it arrives
    row_df
});
```

---

## 📊 Examples

See `examples.zn` for complete examples:

1. **Basic Operations** - Creating Series and DataFrames
2. **CSV I/O** - Reading and writing files
3. **GroupBy** - Aggregation by groups
4. **Merge/Join** - Combining DataFrames
5. **Filtering** - Boolean indexing
6. **Missing Data** - Null value handling

Run all examples:
```nova
use dataframe::examples::run_all_examples;

run_all_examples();
```

---

## 🏗️ Architecture

```
dataframe/
├── mod.zn              # Public API
├── core.zn             # Series, DataFrame, Index
├── dtype.zn            # Type system
├── groupby.zn          # GroupBy operations
├── merge.zn            # Join/merge operations
├── ops.zn              # General operations
├── io/
│   ├── mod.zn
│   ├── csv.zn          # CSV I/O
│   └── json.zn         # JSON I/O
└── examples.zn         # Usage examples
```

---

## 🔬 Implementation Status

### ✅ Implemented
- Series with statistics (sum, mean, min, max, std)
- DataFrame structure
- RangeIndex
- CSV/JSON I/O (basic)
- GroupBy structure
- Merge/Join structure
- Null value handling
- Examples

### 🚧 In Progress
- Full type inference
- Advanced indexing (.loc, .iloc)
- Pivot tables
- Time series support
- MultiIndex

### 📋 Planned
- Parquet I/O
- SQL integration
- Categorical data optimization
- Memory optimization
- Parallel operations
- GPU acceleration

---

## 🆚 Comparison with Pandas

| Feature | Pandas | Nova DataFrame |
|---------|--------|----------------|
| **Type Safety** | Dynamic | Compile-time checked |
| **Null Handling** | Implicit (NaN, None) | Explicit (Nullable<T>) |
| **Unit Support** | Manual | Built-in unit algebra |
| **Performance** | NumPy backend | Tensor + SIMD |
| **Memory** | Row-major | Columnar (optimized) |
| **Syntax** | Python | Nova |

---

## 🚀 Performance

Nova DataFrame is designed for high performance:

- **Columnar Storage** - Cache-friendly data layout
- **Tensor Backend** - Leverage Nova's tensor operations
- **SIMD Operations** - Vectorized numeric operations
- **Zero-Copy** - Minimal data copying
- **Compile-Time Optimization** - Nova's compiler optimizations

---

## 📖 API Reference

### Series<T>

```nova
skill Series<T> {
    fn new(data: Vec<T>) -> Series<T>
    fn with_index(data: Vec<T>, index: Box<dyn Index>) -> Series<T>
    fn with_name(self, name: String) -> Series<T>
    fn len(&self) -> usize
    fn get(&self, pos: usize) -> Option<&Nullable<T>>
    fn at(&self, label: i64) -> Option<&Nullable<T>>
    fn slice(&self, start: usize, end: usize) -> Series<T>
    fn filter(&self, mask: &[bool]) -> Series<T>
    fn count(&self) -> usize
    fn has_nulls(&self) -> bool
    fn dropna(&self) -> Series<T>
}

// For Series<f64>
skill Series<f64> {
    fn sum(&self) -> f64
    fn mean(&self) -> f64
    fn min(&self) -> Option<f64>
    fn max(&self) -> Option<f64>
    fn std(&self) -> f64
}
```

### DataFrame

```nova
skill DataFrame {
    fn new() -> DataFrame
    fn from_columns<T>(data: HashMap<String, Vec<T>>) -> DataFrame
    fn add_column<T>(&mut self, name: String, data: Vec<T>)
    fn len(&self) -> usize
    fn width(&self) -> usize
    fn shape(&self) -> (usize, usize)
    fn columns(&self) -> &[String]
    fn get_column<T>(&self, name: &str) -> Option<Series<T>>
    fn select(&self, cols: &[&str]) -> DataFrame
    fn info(&self)
}
```

---

## 🤝 Contributing

Nova DataFrame is part of the Nova standard library. Contributions are welcome!

See the main Nova repository for contribution guidelines.

---

## 📄 License

Part of the Nova programming language ecosystem.

---

**Built with Nova's powerful type system, unit algebra, and compile-time guarantees.**
