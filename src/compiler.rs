use crate::lexer::{Lexer, Token};
use crate::block::Block;

pub struct Parser<'a> {
    lexer: Lexer<'a>,
    current: Option<Token>,
    previous: Option<Token>
}

impl<'a> Parser<'a> {
    pub fn new(source: &'a String) -> Self {
        Self {
            lexer: Lexer::new(source),
            current: None,
            previous: None
        }
    }
}

pub struct Compiler<'a> {
    parser: Parser<'a>
}

impl<'a> Compiler<'a> {
    pub fn new(source: &'a String) -> Self {
        Self {
            parser: Parser::new(&source)
        }
    }

    pub fn compile(&mut self) -> Block {
        todo!();
    }
}