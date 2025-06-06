use crate::block::{Block, OpCode};
use crate::value::Value;

pub enum InterpretResult {
    Ok,
    CompileError,
    RuntimeError
}

pub struct VM {
    block: Block,
    stack: Vec<Value>
}

impl VM {
    pub fn new() -> Self {
        Self {
            block: Block::new(),
            stack: Vec::new()
        }
    }

    pub fn interpret(&mut self, block: Block) -> InterpretResult {
        self.block = block;
        self.run()
    }

    fn run(&self) -> InterpretResult {
        let mut pc = 0;

        loop {
            pc += 1;
            if let Some(opcode) = OpCode::from_byte(self.block.code[pc - 1]) {
                match opcode {
                    OpCode::Return => return InterpretResult::Ok
                }
            } else {
                return InterpretResult::RuntimeError;
            }
        }
    }
}
