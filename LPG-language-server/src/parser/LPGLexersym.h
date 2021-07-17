
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2007 IBM Corporation.
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v1.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v10.html
//
//Contributors:
//    Philippe Charles (pcharles@us.ibm.com) - initial API and implementation

////////////////////////////////////////////////////////////////////////////////

#pragma once
 #include <vector>
#include<string>
  struct LPGLexersym {
     typedef  unsigned char byte;
      static constexpr int
      Char_CtlCharNotWS = 102,
      Char_LF = 5,
      Char_CR = 6,
      Char_HT = 1,
      Char_FF = 2,
      Char_a = 15,
      Char_b = 40,
      Char_c = 24,
      Char_d = 30,
      Char_e = 7,
      Char_f = 31,
      Char_g = 42,
      Char_h = 50,
      Char_i = 13,
      Char_j = 70,
      Char_k = 44,
      Char_l = 18,
      Char_m = 34,
      Char_n = 28,
      Char_o = 22,
      Char_p = 36,
      Char_q = 56,
      Char_r = 11,
      Char_s = 20,
      Char_t = 9,
      Char_u = 38,
      Char_v = 52,
      Char_w = 53,
      Char_x = 48,
      Char_y = 45,
      Char_z = 68,
      Char__ = 26,
      Char_A = 16,
      Char_B = 41,
      Char_C = 25,
      Char_D = 32,
      Char_E = 8,
      Char_F = 33,
      Char_G = 43,
      Char_H = 51,
      Char_I = 14,
      Char_J = 71,
      Char_K = 46,
      Char_L = 19,
      Char_M = 35,
      Char_N = 29,
      Char_O = 23,
      Char_P = 37,
      Char_Q = 57,
      Char_R = 12,
      Char_S = 21,
      Char_T = 10,
      Char_U = 39,
      Char_V = 54,
      Char_W = 55,
      Char_X = 49,
      Char_Y = 47,
      Char_Z = 69,
      Char_0 = 58,
      Char_1 = 59,
      Char_2 = 60,
      Char_3 = 61,
      Char_4 = 62,
      Char_5 = 63,
      Char_6 = 64,
      Char_7 = 65,
      Char_8 = 66,
      Char_9 = 67,
      Char_AfterASCII = 72,
      Char_Space = 3,
      Char_DoubleQuote = 97,
      Char_SingleQuote = 98,
      Char_Percent = 74,
      Char_VerticalBar = 76,
      Char_Exclamation = 77,
      Char_AtSign = 78,
      Char_BackQuote = 79,
      Char_Tilde = 80,
      Char_Sharp = 92,
      Char_DollarSign = 100,
      Char_Ampersand = 81,
      Char_Caret = 82,
      Char_Colon = 83,
      Char_SemiColon = 84,
      Char_BackSlash = 85,
      Char_LeftBrace = 86,
      Char_RightBrace = 87,
      Char_LeftBracket = 93,
      Char_RightBracket = 94,
      Char_QuestionMark = 73,
      Char_Comma = 4,
      Char_Dot = 88,
      Char_LessThan = 99,
      Char_GreaterThan = 95,
      Char_Plus = 89,
      Char_Minus = 27,
      Char_Slash = 90,
      Char_Star = 91,
      Char_LeftParen = 96,
      Char_RightParen = 75,
      Char_Equal = 17,
      Char_EOF = 101;

      inline const static std::vector<std::wstring> orderedTerminalSymbols = {
                 L"",
                 L"HT",
                 L"FF",
                 L"Space",
                 L"Comma",
                 L"LF",
                 L"CR",
                 L"e",
                 L"E",
                 L"t",
                 L"T",
                 L"r",
                 L"R",
                 L"i",
                 L"I",
                 L"a",
                 L"A",
                 L"Equal",
                 L"l",
                 L"L",
                 L"s",
                 L"S",
                 L"o",
                 L"O",
                 L"c",
                 L"C",
                 L"_",
                 L"Minus",
                 L"n",
                 L"N",
                 L"d",
                 L"f",
                 L"D",
                 L"F",
                 L"m",
                 L"M",
                 L"p",
                 L"P",
                 L"u",
                 L"U",
                 L"b",
                 L"B",
                 L"g",
                 L"G",
                 L"k",
                 L"y",
                 L"K",
                 L"Y",
                 L"x",
                 L"X",
                 L"h",
                 L"H",
                 L"v",
                 L"w",
                 L"V",
                 L"W",
                 L"q",
                 L"Q",
                 L"0",
                 L"1",
                 L"2",
                 L"3",
                 L"4",
                 L"5",
                 L"6",
                 L"7",
                 L"8",
                 L"9",
                 L"z",
                 L"Z",
                 L"j",
                 L"J",
                 L"AfterASCII",
                 L"QuestionMark",
                 L"Percent",
                 L"RightParen",
                 L"VerticalBar",
                 L"Exclamation",
                 L"AtSign",
                 L"BackQuote",
                 L"Tilde",
                 L"Ampersand",
                 L"Caret",
                 L"Colon",
                 L"SemiColon",
                 L"BackSlash",
                 L"LeftBrace",
                 L"RightBrace",
                 L"Dot",
                 L"Plus",
                 L"Slash",
                 L"Star",
                 L"Sharp",
                 L"LeftBracket",
                 L"RightBracket",
                 L"GreaterThan",
                 L"LeftParen",
                 L"DoubleQuote",
                 L"SingleQuote",
                 L"LessThan",
                 L"DollarSign",
                 L"EOF",
                 L"CtlCharNotWS"
             };

     static constexpr  int numTokenKinds = 103;
     static constexpr  bool isValidForParser = true;
};
