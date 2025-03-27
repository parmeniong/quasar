use std::str::Chars;
use std::iter::Peekable;

#[derive(Debug)]
pub enum TokenType {
    String(String), Char(char), Int(u32), Float(f32), Bool(bool), Identifier(String), Null,
    LeftParenthesis, RightParenthesis, LeftBrace, RightBrace,
    Comma, Dot, Semicolon, Arrow,
    Plus, Minus, Asterisk, Slash,
    PlusEqual, MinusEqual, AsteriskEqual, SlashEqual,
    Equal, DoubleEqual, NotEqual,
    Greater, GreaterEqual, Less, LessEqual,
    And, Or, Not, If, Then, Else, For, In, Do, Loop, Match, Let, Const, Fn, Return
}

#[derive(Debug)]
pub struct Token {
    token_type: TokenType,
    line: u32,
    column: u32,
    length: usize
}

#[derive(Debug)]
pub enum LexerErrorType {
    UnexpectedCharacter(char, Option<char>),
    MultipleDecimalPoints,
    UnterminatedString,
    InvalidEscapeSequence,
    UnterminatedCharacter,
    EmptyCharacter
}

#[derive(Debug)]
pub struct LexerError {
    error_type: LexerErrorType,
    line: u32,
    column: u32,
    length: usize
}

pub struct Lexer<'a> {
    _source: &'a String,
    chars: Peekable<Chars<'a>>,
    line: u32,
    column: u32
}

impl<'a> Lexer<'a> {
    pub fn new(source: &'a String) -> Self {
        Self {
            _source: source,
            chars: source.chars().peekable(),
            line: 0,
            column: 0
        }
    }

    pub fn get_token(&mut self) -> Result<Option<Token>, LexerError> {
        self.skip_whitespace();

        if self.is_at_end() {
            return Ok(None)
        }

        let c = self.advance();

        match c {
            '(' => Ok(Some(self.make_token(TokenType::LeftParenthesis, 1))),
            ')' => Ok(Some(self.make_token(TokenType::RightParenthesis, 1))),
            '{' => Ok(Some(self.make_token(TokenType::LeftBrace, 1))),
            '}' => Ok(Some(self.make_token(TokenType::RightBrace, 1))),
            '.' => Ok(Some(self.make_token(TokenType::Dot, 1))),
            ',' => Ok(Some(self.make_token(TokenType::Comma, 1))),
            ';' => Ok(Some(self.make_token(TokenType::Semicolon, 1))),
            '+' => Ok(Some(
                if self.match_char('=') {
                    self.make_token(TokenType::PlusEqual, 2)
                } else {
                    self.make_token(TokenType::Plus, 1)
                }
            )),
            '-' => Ok(Some(
                if self.match_char('=') {
                    self.make_token(TokenType::MinusEqual, 2)
                } else {
                    self.make_token(TokenType::Minus, 1)
                }
            )),
            '*' => Ok(Some(
                if self.match_char('=') {
                    self.make_token(TokenType::AsteriskEqual, 2)
                } else {
                    self.make_token(TokenType::Asterisk, 1)
                }
            )),
            '/' => Ok(Some(
                if self.match_char('=') {
                    self.make_token(TokenType::SlashEqual, 2)
                } else {
                    self.make_token(TokenType::Slash, 1)
                }
            )),
            '=' => Ok(Some(
                if self.match_char('=') {
                    self.make_token(TokenType::DoubleEqual, 2)
                } else if self.match_char('>') {
                    self.make_token(TokenType::Arrow, 2)
                } else {
                    self.make_token(TokenType::Equal, 1)
                }
            )),
            '>' => Ok(Some(
                if self.match_char('=') {
                    self.make_token(TokenType::GreaterEqual, 2)
                } else {
                    self.make_token(TokenType::Greater, 1)
                }
            )),
            '<' => Ok(Some(
                if self.match_char('=') {
                    self.make_token(TokenType::LessEqual, 2)
                } else {
                    self.make_token(TokenType::Less, 1)
                }
            )),
            '!' => {
                if self.match_char('=') {
                    Ok(Some(self.make_token(TokenType::NotEqual, 2)))
                } else {
                    Err(self.make_error(
                        LexerErrorType::UnexpectedCharacter(c, Some('=')),
                        self.line,
                        self.column,
                        1
                    ))
                }
            },
            '0'..='9' => self.number(c),
            '"' => self.string(),
            '\'' => self.char(),
            'a'..='z' | 'A'..='Z' | '_' => self.identifier(c),
            _ => Err(self.make_error(
                LexerErrorType::UnexpectedCharacter(c, None),
                self.line,
                self.column,
                1
            ))
        }
    }

    fn number(&mut self, first_char: char) -> Result<Option<Token>, LexerError> {
        let mut value = String::new();
        value.push(first_char);
        let mut is_float = false;

        while let Some(c) = self.peek() {
            match *c {
                '0'..='9' => {
                    value.push(*c);
                    self.advance();
                },
                '.' => {
                    if is_float {
                        return Err(self.make_error(
                            LexerErrorType::MultipleDecimalPoints,
                            self.line,
                            self.column + 1,
                            1
                        ));
                    }
                    is_float = true;
                    value.push(*c);
                    self.advance();
                }
                _ => break
            }
        }

        if is_float {
            Ok(Some(self.make_token(TokenType::Float(value.parse().unwrap()), value.len())))
        } else {
            Ok(Some(self.make_token(TokenType::Int(value.parse().unwrap()), value.len())))
        }
    }

    fn string(&mut self) -> Result<Option<Token>, LexerError> {
        let mut value = String::new();
        let mut terminated = false;
        let mut has_newline = false;
        let mut length = 0;

        loop {
            if self.is_at_end() {
                break;
            }

            let c = self.advance();

            if c == '\n' {
                has_newline = true;
                break;
            }

            if c == '"' {
                terminated = true;
                break;
            }

            if c == '\\' {
                if self.is_at_end() {
                    return Err(self.make_error(
                        LexerErrorType::UnterminatedString,
                        self.line,
                        self.column + 1,
                        1
                    ));
                }

                let next = self.advance();
                length += 2;

                match next {
                    'n' => value.push('\n'),
                    't' => value.push('\t'),
                    'r' => value.push('\r'),
                    '\\' => value.push('\\'),
                    '"' => value.push('"'),
                    _ => return Err(self.make_error(
                        LexerErrorType::InvalidEscapeSequence,
                        self.line,
                        self.column - 1,
                        2
                    ))
                }
            } else {
                value.push(c);
                length += 1;
            }
        }

        if terminated {
            Ok(Some(self.make_token(TokenType::String(value.clone()), length + 2)))
        } else {
            Err(self.make_error(
                LexerErrorType::UnterminatedString,
                self.line,
                if has_newline { self.column } else { self.column + 1 },
                1
            ))
        }
    }

    fn char(&mut self) -> Result<Option<Token>, LexerError> {
        if self.is_at_end() {
            return Err(self.make_error(
                LexerErrorType::UnterminatedCharacter,
                self.line,
                self.column + 1,
                1
            ));
        }

        let c = self.advance();
        let mut length = 0;
        let mut terminated = false;

        let char_value = if c == '\\' {
            length += 1;
            if self.is_at_end() {
                return Err(self.make_error(
                    LexerErrorType::UnterminatedCharacter,
                    self.line,
                    self.column,
                    1
                ));
            }

            let escaped = self.advance();
            length += 1;

            match escaped {
                'n' => '\n',
                't' => '\t',
                'r' => '\r',
                '\\' => '\\',
                '\'' => '\'',
                _ => return Err(self.make_error(
                    LexerErrorType::InvalidEscapeSequence,
                    self.line,
                    self.column - 1,
                    2
                ))
            }
        } else if c == '\'' {
            return Err(self.make_error(
                LexerErrorType::EmptyCharacter,
                self.line,
                self.column - 1,
                2
            ));
        } else if c == '\n' {
            return Err(self.make_error(
                LexerErrorType::UnterminatedCharacter,
                self.line,
                self.column,
                1
            ));
        } else {
            length += 1;
            c
        };

        if self.is_at_end() {
            return Err(self.make_error(
                LexerErrorType::UnterminatedCharacter,
                self.line,
                self.column + 1,
                1
            ));
        }

        if self.advance() == '\'' {
            terminated = true;
        }

        if terminated {
            Ok(Some(self.make_token(TokenType::Char(char_value), length + 2)))
        } else {
            Err(self.make_error(
                LexerErrorType::UnterminatedCharacter,
                self.line,
                self.column,
                1
            ))
        }
    }

    fn identifier(&mut self, first_char: char) -> Result<Option<Token>, LexerError> {
        let mut value = String::new();
        value.push(first_char);

        loop {
            if self.is_at_end() {
                break;
            }

            let c = self.peek().unwrap();
            if c.is_ascii_alphanumeric() || *c == '_' {
                value.push(self.advance());
            } else {
                break;
            }
        }

        match value.as_str() {
            "true" => Ok(Some(self.make_token(TokenType::Bool(true), 4))),
            "false" => Ok(Some(self.make_token(TokenType::Bool(false), 5))),
            "null" => Ok(Some(self.make_token(TokenType::Null, 4))),
            "and" => Ok(Some(self.make_token(TokenType::And, 3))),
            "or" => Ok(Some(self.make_token(TokenType::Or, 2))),
            "not" => Ok(Some(self.make_token(TokenType::Not, 3))),
            "if" => Ok(Some(self.make_token(TokenType::If, 2))),
            "then" => Ok(Some(self.make_token(TokenType::Then, 4))),
            "else" => Ok(Some(self.make_token(TokenType::Else, 4))),
            "for" => Ok(Some(self.make_token(TokenType::For, 3))),
            "in" => Ok(Some(self.make_token(TokenType::In, 2))),
            "do" => Ok(Some(self.make_token(TokenType::Do, 2))),
            "loop" => Ok(Some(self.make_token(TokenType::Loop, 4))),
            "match" => Ok(Some(self.make_token(TokenType::Match, 5))),
            "let" => Ok(Some(self.make_token(TokenType::Let, 3))),
            "const" => Ok(Some(self.make_token(TokenType::Const, 5))),
            "fn" => Ok(Some(self.make_token(TokenType::Fn, 2))),
            "return" => Ok(Some(self.make_token(TokenType::Return, 6))),
            _ => Ok(Some(self.make_token(TokenType::Identifier(value.clone()), value.len()))),
        }
    }

    fn is_at_end(&mut self) -> bool {
        self.chars.peek() == None
    }

    fn advance(&mut self) -> char {
        self.column += 1;
        self.chars.next().unwrap()
    }

    fn peek(&mut self) -> Option<&char> {
        self.chars.peek()
    }

    fn match_char(&mut self, expected: char) -> bool {
        if self.is_at_end() {
            return false;
        }
        if *self.chars.peek().unwrap() != expected {
            return false;
        }
        self.advance();
        true
    }

    fn skip_whitespace(&mut self) {
        loop {
            if self.is_at_end() {
                return;
            }

            let c = self.peek().unwrap();
            match *c {
                ' ' | '\t' => {
                    self.advance();
                },
                '\n' => {
                    self.advance();
                    self.column = 0;
                    self.line += 1;
                },
                _ => return
            }
        }
    }

    fn make_token(&self, token_type: TokenType, length: usize) -> Token {
        Token {
            token_type,
            line: self.line,
            column: self.column - length as u32,
            length
        }
    }

    fn make_error(
        &self,
        error_type: LexerErrorType,
        line: u32,
        column: u32,
        length: usize
    ) -> LexerError {
        LexerError {
            error_type,
            line,
            column: column - 1,
            length
        }
    }
}