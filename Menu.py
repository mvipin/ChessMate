import os
import time
import threading

from LCD import SSD1306
from RPi import GPIO
from PIL import Image, ImageDraw, ImageFont

class Menu:
    def __init__(self, pychess, options0=[], options1=[], options2=[], options3=[], options4=[], options5=[]):
        self.pychess = pychess
        self.options = options0
        self.options1 = options1
        self.options2 = options2
        self.options3 = options3
        self.options4 = options4
        self.options5 = options5
        self.moveString = "Your Turn..."
        self.rowCount = 3
        self.reset_menu()

        self.oled = SSD1306.SSD1306_128_32(rst=None, gpio=GPIO, i2c_bus=11)
        self.oled.begin()
        self.oled.clear()
        self.oled.display()

        self.image = Image.new('1', (self.oled.width, self.oled.height))
        self.draw = ImageDraw.Draw(self.image)
        self.font = ImageFont.truetype(os.path.dirname(__file__) +
                '/fonts/pixel_arial_11.ttf', 8)
        self.bigfont = ImageFont.truetype(os.path.dirname(__file__) +
                '/fonts/pixel_arial_11.ttf', 15)

        self.renderThread = None
        self.overrideMove = ""
        pychess.install_menu_ref(self)

    def reset_menu(self):
        self.menuOption = None # Menu
        self.submenuOption = [
                0, # unused (new game)
                0, # unused (new puzzle)
                0, # Time - Computer
                0, # Time - Human
                0, # Color
                0, # Level
                0, # unused (shutdown)
                0, # Rank
                0, # File
                0] # Confirm
        self.menulevel = 0

    def set_options(self, options):
        self.options = options
        self.menuOption = None

    def set_selection(self, highlight):
        if highlight is None:
            self.menuOption = None
        elif highlight < 0:
            self.menuOption = 0
        elif highlight >= len(self.options):
            self.menuOption = len(self.options) - 1
        else:
            self.menuOption = highlight

    def set_subselection(self, highlight, high):
        self.submenuOption[self.menuOption] = highlight
        if self.submenuOption[self.menuOption] < 0:
            self.submenuOption[self.menuOption] = 0
        elif self.submenuOption[self.menuOption] > high:
            self.submenuOption[self.menuOption] = high

    def change_selection(self, by):
        if self.menulevel == 0:
            self.set_selection(0 if self.menuOption is None else
                    self.menuOption + by)
        else:
            if self.menuOption == 5: # Level
                self.set_subselection(self.submenuOption[5] + by,
                        len(self.options1) - 1)
            elif self.menuOption == 2: # Time - Computer
                self.set_subselection(self.submenuOption[2] + by, 30)
            elif self.menuOption == 3: # Time - Human
                self.set_subselection(self.submenuOption[3] + by * 10, 300)
            elif self.menuOption == 4: # Color
                self.set_subselection(self.submenuOption[4] + by, 1)
            elif self.menuOption == 7: # Rank
                self.set_subselection(self.submenuOption[7] + by,
                        len(self.options3) - 1)
            elif self.menuOption == 8: # File
                self.set_subselection(self.submenuOption[8] + by,
                        len(self.options4) - 1)
            elif self.menuOption == 9: # Confirm
                self.set_subselection(self.submenuOption[9] + by, 1)

    def process_submenu_selection(self):
        if self.menuOption == 7:
            self.menulevel = 1
            self.menuOption = 8
            self.overrideMove += self.options3[self.submenuOption[7]]
            if len(self.overrideMove) == 4:
                self.show_game_status(self.overrideMove)
                self.menulevel = 1
                self.menuOption = 9
        elif self.menuOption == 8:
            self.menulevel = 1
            self.menuOption = 7
            self.overrideMove += self.options4[self.submenuOption[8]]
            if len(self.overrideMove) == 4:
                self.show_game_status(self.overrideMove)
                self.menulevel = 1
                self.menuOption = 9
        elif self.menuOption == 9:
            if self.options5[self.submenuOption[9]] == "confirm":
                self.overrideMove += '\0'
                return False
            else:
                self.menulevel = 1
                self.menuOption = 7
                self.overrideMove = ""
        return True

    def change_level(self):
        # Toggle between menu and submenu
        render = True
        if self.menulevel:
            self.menulevel = 0
            render = self.process_submenu_selection()
        else:
            self.menulevel = 1
        if render:
            self.render()

    def get_manual_override(self):
        self.show_game_status("enter move")
        time.sleep(1)
        self.menulevel = 1
        self.menuOption = 7
        self.render()
        self.overrideMove = ""
        while True:
            if len(self.overrideMove) >= 5:
                break
        return self.overrideMove

    def blank(self, draw=False):
        self.draw.rectangle((-1, -1, self.oled.width+1, self.oled.height+1),
                outline=0, fill=0)
        if draw:
            self.oled.image(self.image)
            self.oled.display()

    def render(self):
        if self.renderThread is None or not self.renderThread.is_alive():
            self.renderThread = threading.Thread(target=self.__render)
            self.renderThread.start()

    def show_game_status(self, text):
        self.blank()
        self.draw.text((3, 8), text, font=self.bigfont, fill=1)
        self.oled.image(self.image)
        self.oled.display()

    def __render(self):
        self.blank()
        self.__build()
        self.oled.image(self.image)
        self.oled.display()

    def __build(self):
        if self.menulevel == 0:
            self.__build_menu()
        else:
            self.__build_sub_menu()

    def __build_menu(self):
        # adjust the start/end positions of the range
        if (self.menuOption is None) or (self.menuOption < self.rowCount):
            start = 0
            end = self.rowCount
        elif self.menuOption >= (len(self.options) - self.rowCount):
            end = len(self.options)
            start = end - self.rowCount
        else:
            start = self.menuOption
            end = start + self.rowCount

        # draw the menu options
        top = 0
        for x in range(start, end):
            fill = 1
            if self.menuOption is not None and self.menuOption == x:
                self.draw.rectangle([0, top, self.oled.width, top + 11],
                        outline=0, fill=1)
                fill = 0
            self.draw.text((3, top + 1), self.options[x], font=self.font,
                    fill=fill)
            top += 10

    def __build_sub_menu(self):
        text = ""
        new = False
        puzzle = False
        skill = 0
        human_first = True
        if self.menuOption == 5:
            text = self.options1[self.submenuOption[5]]
        elif self.menuOption == 2:
            text = str(self.submenuOption[2]) + " seconds"
        elif self.menuOption == 3:
            text = str(self.submenuOption[3]) + " seconds"
        elif self.menuOption == 4:
            text = self.options2[self.submenuOption[4]]
        elif self.menuOption == 6:
            text = "resetting ..."
            self.pychess.reset_target()
        elif self.menuOption == 0:
            new = True
            text = "USER(W): "
            if self.submenuOption[4] == 1:
                human_first = False
                text = "COMP(W): "
            skill = self.submenuOption[5]
            text = text + str(skill)
        elif self.menuOption == 1:
            puzzle = True
            text = "USER(W): "
            human_first = True
            skill = self.submenuOption[5]
            text = text + str(skill)
        elif self.menuOption == 7:
            text = self.options3[self.submenuOption[7]]
        elif self.menuOption == 8:
            text = self.options4[self.submenuOption[8]]
        elif self.menuOption == 9:
            text = self.options5[self.submenuOption[9]] + " " + self.overrideMove
        self.draw.text((3, 8), text, font=self.bigfont, fill=1)
        if new:
            self.pychess.setup_game(human_first, skill)
        if puzzle:
            self.pychess.setup_puzzle(human_first, skill)
