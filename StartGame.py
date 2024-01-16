import time
import serial
import schedule
from timeloop import Timeloop
from datetime import timedelta
from Menu import Menu
from Rotary import Rotary
from ChessSoft import ChessSoft

# Setup serial
ser = serial.Serial(port='/dev/ttyS0',baudrate=9600,parity=serial.PARITY_NONE,stopbits=serial.STOPBITS_ONE,bytesize=serial.EIGHTBITS)
ser.write("init\n".encode('utf8'))

c = ChessSoft(ser)

m = Menu(c,
    ["LEVEL",
    "TIME - COMPUTER",
    "TIME - HUMAN",
    "COLOR - HUMAN",
    "NEW GAME",
    "SHUTDOWN CHESS"],
    ["SUPER EASY",
    "EASY",
    "MEDIUM",
    "HARD",
    "SUPER HARD",
    "IMPOSSIBLE"],
    ["WHITE",
    "BLACK"])

r = Rotary(**{'menu': m, 'clk': 29, 'dt': 31, 'btn': 37})

m.set_selection(0)
m.render()

def job():
    if c.is_game_set():
        if c.game_over():
            return schedule.CancelJob
        c.play_next_move()

#schedule.every(5).seconds.do(job)
schedule.every(1).seconds.do(job)

while True:
    schedule.run_pending()
    #time.sleep(1)
