pub enum ValueType {
    Int
}

union ValueUnion {
    int: i32
}

pub struct Value {
    value_type: ValueType,
    value: ValueUnion
}

impl Value {
    pub fn from_int(int: i32) -> Self {
        Self {
            value_type: ValueType::Int,
            value: ValueUnion {
                int
            }
        }
    }

    pub fn as_int(&self) -> i32 {
        unsafe {
            self.value.int
        }
    }
}
