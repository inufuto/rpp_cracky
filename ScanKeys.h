constexpr uint8_t Keys_Left = 0x01;
constexpr uint8_t Keys_Right = 0x02;
constexpr uint8_t Keys_DirX = Keys_Left | Keys_Right;
constexpr uint8_t Keys_Up = 0x04;
constexpr uint8_t Keys_Down = 0x08;
constexpr uint8_t Keys_DirY = Keys_Up | Keys_Down;
constexpr uint8_t Keys_Dir = Keys_DirX | Keys_DirY;
constexpr uint8_t Keys_Button0 = 0x10;
constexpr uint8_t Keys_Button1 = 0x20;

extern void InitKeys();
extern uint8_t ScanKeys();
