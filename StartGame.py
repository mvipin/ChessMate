import time
import serial
import schedule
from timeloop import Timeloop
from datetime import timedelta
from Menu import Menu
from Rotary import Rotary
from ChessSoft import ChessSoft

# Setup serial
ser = serial.Serial(port='/dev/ttyAMA0',baudrate=9600,parity=serial.PARITY_NONE,stopbits=serial.STOPBITS_ONE,bytesize=serial.EIGHTBITS)
ser.write("init\n".encode('utf8'))
ser.flush()

c = ChessSoft(ser)

m = Menu(c,
    ["NEW GAME",
    "TIME - COMPUTER",
    "TIME - HUMAN",
    "COLOR - HUMAN",
    "LEVEL",
    "RESET CHESS"],
    ["IMPOSSIBLE",
    "SUPER HARD",
    "HARD",
    "MEDIUM",
    "EASY",
    "SUPER EASY"],
    ["WHITE",
    "BLACK"],
    ["a", "b", "c", "d", "e", "f", "g", "h"],
    ["1", "2", "3", "4", "5", "6", "7", "8"],
    ["confirm","discard"])

r = Rotary(**{'menu': m, 'clk': 29, 'dt': 31, 'btn': 37})

m.set_selection(0)
m.render()

def job():
    if c.is_game_set():
        if c.game_over():
            c.show_result()
            c.reset_board()
            m.reset_menu()
            m.set_selection(0)
            return m.render()
        c.play_next_move()

schedule.every(0.1).seconds.do(job)

while True:
    schedule.run_pending()
    #time.sleep(1)
