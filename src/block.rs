pub enum OpCode {
    Return
}

pub struct Block {
    code: Vec<u8>
}

impl Block {
    pub fn new() -> Self {
        Self {
            code: Vec::new()
        }
    }

    pub fn push(&mut self, byte: u8) {
        self.code.push(byte);
    }
}
