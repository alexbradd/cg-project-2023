#pragma once

namespace seng {

/**
 * Possible keycodes that can be received from keyboard events
 *
 * Straight up copy-pasted from GLFW's documentation, even with the same values
 * so we have easy conversion between the twos.
 */
enum struct KeyCode {
  eUnknown = -1,
  eSpace = 32,
  eApostrophe = 39,
  eComma = 44,
  eMinus = 45,
  ePeriod = 46,
  eSlash = 47,
  eNum0 = 48,
  eNum1 = 49,
  eNum2 = 50,
  eNum3 = 51,
  eNum4 = 52,
  eNum5 = 53,
  eNum6 = 54,
  eNum7 = 55,
  eNum8 = 56,
  eNum9 = 57,
  eSemicolon = 59,
  eEqual = 61,
  eKeyA = 65,
  eKeyB = 66,
  eKeyC = 67,
  eKeyD = 68,
  eKeyE = 69,
  eKeyF = 70,
  eKeyG = 71,
  eKeyH = 72,
  eKeyI = 73,
  eKeyJ = 74,
  eKeyK = 75,
  eKeyL = 76,
  eKeyM = 77,
  eKeyN = 78,
  eKeyO = 79,
  eKeyP = 80,
  eKeyQ = 81,
  eKeyS = 83,
  eKeyT = 84,
  eKeyU = 85,
  eKeyV = 86,
  eKeyW = 87,
  eKeyX = 88,
  eKeyY = 89,
  eKeyZ = 90,
  eLeftBracket = 91,  /* [ */
  eBasckslash = 92,   /* \ */
  eRightBracket = 93, /* ] */
  eGrave = 96,        /* ` */
  eEsc = 256,
  eEnter = 257,
  eTab = 258,
  eBackspace = 259,
  eInsert = 260,
  eDelete = 261,
  eRight = 262,
  eLeft = 263,
  eDown = 264,
  eUp = 265,
  ePageUp = 266,
  ePageDown = 267,
  eHome = 268,
  eEnd = 269,
  eCapsLock = 280,
  eScrollLock = 281,
  eNumLock = 282,
  ePrintScreen = 283,
  ePause = 284,
  eF1 = 290,
  eF2 = 291,
  eF3 = 292,
  eF4 = 293,
  eF5 = 294,
  eF6 = 295,
  eF7 = 296,
  eF8 = 297,
  eF9 = 298,
  eF10 = 299,
  eF11 = 300,
  eF12 = 301,
  eF13 = 302,
  eF14 = 303,
  eF15 = 304,
  eF16 = 305,
  eF17 = 306,
  eF18 = 307,
  eF19 = 308,
  eF20 = 309,
  eF21 = 310,
  eF22 = 311,
  eF23 = 312,
  eF24 = 313,
  eF25 = 314,
  eKeypad0 = 320,
  eKeypad1 = 321,
  eKeypad2 = 322,
  eKeypad3 = 323,
  eKeypad4 = 324,
  eKeypad5 = 325,
  eKeypad6 = 326,
  eKeypad7 = 327,
  eKeypad8 = 328,
  eKeypad9 = 329,
  eKeypadDecimal = 330,
  eKeypadDivide = 331,
  eKeypadMultiply = 332,
  eKeypadSubtract = 333,
  eKeypadAdd = 334,
  eKeypadEnter = 335,
  eKeypadEqual = 336,
  eModLeftShift = 340,
  eModLeftControl = 341,
  eModLeftAlt = 342,
  eModLeft_SUPER = 343,
  eModRightShift = 344,
  eModRightControl = 345,
  eModRightAlt = 346,
  eModRightSuper = 347,
  eModMenu = 348
};

/**
 * Possible key states which we can receive.
 *
 * Straight up copy-pasted from GLFW's documentation, even with the same values
 * so we have easy conversion between the twos.
 */
enum struct KeyEvent {
  eRelease = 0,
  ePress = 1,
  eRepeat = 2,
};

}  // namespace seng
