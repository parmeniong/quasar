use crate::lexer::Token;

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

pub struct Compiler;

impl Compiler {
    pub fn new() -> Self {
        Self
    }
}