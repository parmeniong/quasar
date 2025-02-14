use crate::value::Value;

pub enum OpCode {
    Return
}

impl OpCode {
    pub fn from_byte(byte: u8) -> Option<Self> {
        match byte {
            0x00 => Some(Self::Return),
            _ => None
        }
    }
}

pub struct Block {
    code: Vec<u8>,
    constants: Vec<Value>
}

impl Block {
    pub fn new() -> Self {
        Self {
            code: Vec::new(),
            constants: Vec::new()
        }
    }

    pub fn push(&mut self, byte: u8) {
        self.code.push(byte);
    }

    pub fn push_constant(&mut self, value: Value) {
        self.constants.push(value);
    }

    #[cfg(debug_assertions)]
    pub fn disassemble(&self) {
        for offset in 0..self.code.len() {
            print!("{:04X} ", offset);

            let byte = self.code[offset];
            if let Some(opcode) = OpCode::from_byte(byte) {
                match opcode {
                    OpCode::Return => println!("RETURN")
                }
            } else {
                println!("{:02X}", byte);
            }
        }
    }
}
