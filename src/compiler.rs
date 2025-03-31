use crate::lexer::{Lexer, Token};
use crate::block::Block;

pub struct Parser {
    current: Option<Token>,
    previous: Option<Token>
}

impl Parser {
    pub fn new() -> Self {
        Self {
            current: None,
            previous: None
        }
    }
}

pub struct Compiler<'a> {
    lexer: Option<Lexer<'a>>
}

impl<'a> Compiler<'a> {
    pub fn new() -> Self {
        Self {
            lexer: None
        }
    }

    pub fn compile(&mut self, source: &'a String) -> Block {
        self.lexer = Some(Lexer::new(source));
        todo!();
    }
}