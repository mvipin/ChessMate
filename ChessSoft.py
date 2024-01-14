import time
import asyncio
import random
import chess
import chess.engine


class ChessSoft:
    def __init__(self, ser):
        self.board = None
        self.menu = None
        self.player1 = None
        self.player2 = None
        self.depth = 0
        self.time = 0.0
        self.serial = ser

    def random_player(self):
        move = random.choice(list(self.board.legal_moves))
        return move.uci()

    async def stockfish_player_async(self) -> None:
        transport, engine = await chess.engine.popen_uci("stockfish")
        result = await engine.play(self.board, chess.engine.Limit(
            time=self.time, depth=self.depth))
        uci = result.move.uci()
        if self.menu != None:
            self.menu.show_game_status(uci)
        await engine.quit()
        return uci

    def stockfish_player(self):
        asyncio.set_event_loop_policy(chess.engine.EventLoopPolicy())
        uci = asyncio.run(self.stockfish_player_async())
        return uci

    async def get_hint_async(self) -> None:
        transport, engine = await chess.engine.popen_uci("stockfish")
        info = await engine.analyse(self.board, chess.engine.Limit(time=0.1))
        print("Score:", info["score"], info["pv"][0])
        await engine.quit()
        return info["pv"][0]

    def get_hint(self):
        asyncio.set_event_loop_policy(chess.engine.EventLoopPolicy())
        move = asyncio.run(self.get_hint_async())
        return move

    def process_start_cmd(self, start):
        # Invalidate move if:
        # - chessboard detects the movement from a non-occupied square
        # - movement from a square with opponent's piece
        # - No moves possible with the piece at the given square
        color = self.board.color_at(chess.parse_square(start))
        legal_uci_moves = [move.uci() for move in self.board.legal_moves]
        print("Legal moves: " + (",".join(sorted(legal_uci_moves))))
        possible_moves = list(filter(lambda x: start in x, legal_uci_moves))
        print("Possible moves: " + str(possible_moves))
        if color != self.board.turn or color == None or not possible_moves:
            #print("Invalid move")
            return None
        possible_squares = [sub.replace(start, '') for sub in possible_moves]
        print("Possible squares: " + str(possible_squares))
        for sq in possible_squares:
            print(chess.parse_square(sq))
        return possible_squares

    def process_stop_cmd(self, start, stop, sq):
        if stop not in sq:
            #print("Invalid move")
            return None
        return start+stop

    def get_pmove(self):
        self.serial.write("scan:start\n".encode('utf8'))
        while True:
            if self.serial.inWaiting():
                line=self.serial.readline()
                break
        start = line.decode('utf8').strip()
        print(start)
        #start = input("start: ")
        possible_squares = self.process_start_cmd(start)
        if possible_squares == None:
            return None
        legal = ':'.join(possible_squares)
        legal = "legal:" + legal + "\n"
        print(legal)
        self.serial.write(legal.encode('utf8'))
        while True:
            if self.serial.inWaiting():
                line=self.serial.readline()
                break
        stop = line.decode('utf8').strip()
        print(start)
        #stop = input("stop: ")
        move = self.process_stop_cmd(start, stop, possible_squares)
        return move

    def get_move(self, prompt):
        legal_uci_moves = [move.uci() for move in self.board.legal_moves]
        uci = input(prompt)
        if uci and uci[0] == "q":
            raise KeyboardInterrupt()
        if uci and uci[0] == "h":
            move = self.get_hint()
            uci = move.uci()
        if uci and uci[0] == "p":
            uci = self.get_pmove()
        try:
            chess.Move.from_uci(uci)
            if uci not in legal_uci_moves:
                uci = None
        except:
            uci = None
        if uci == None:
            print("Invalid move")
        return uci

    def human_player(self):
        uci = None
        while uci == None:
            uci = self.get_move(
                "%s's move[q to quit]> " % self.who(self.board.turn))
        return uci

    def who(self, player):
        return "White" if player == chess.WHITE else "Black"

    def install_menu_ref(self, m):
        self.menu = m

    def update_skill(self, skill):
        if skill <= 0:
            self.depth = 5
            self.time = 0.05
        elif skill == 1:
            self.depth = 5
            self.time = 0.1
        elif skill == 2:
            self.depth = 5
            self.time = 0.2
        elif skill == 3:
            self.depth = 8
            self.time = 0.4
        elif skill == 4:
            self.depth = 13
            self.time = 0.5
        elif skill >= 5:
            self.depth = 22
            self.time = 1.0

    def get_result(self):
        result = None
        msg = None
        if self.board.is_checkmate():
            msg = "checkmate: " + self.who(not self.board.turn) + " wins!"
            result = not self.board.turn
        elif self.board.is_stalemate():
            msg = "draw: stalemate"
        elif self.board.is_fivefold_repetition():
            msg = "draw: 5-fold repetition"
        elif self.board.is_insufficient_material():
            msg = "draw: insufficient material"
        elif self.board.can_claim_draw():
            msg = "draw: claim"
        return (result, msg)

    def update_players(self, human_white):
        if human_white == True:
            self.player1 = self.human_player
            self.player2 = self.stockfish_player
        else:
            self.player1 = self.stockfish_player
            self.player2 = self.human_player

    def setup_game(self, human_white=True, skill=0, pause=0.1):
        self.update_skill(skill)
        self.update_players(human_white)
        self.board = chess.Board()

    def is_game_set(self):
        return self.board != None

    def game_over(self):
        return self.board.is_game_over(claim_draw=True)

    def play_next_move(self):
        if self.board.turn == chess.WHITE:
            uci = self.player1()
        else:
            uci = self.player2()
        #name = self.who(self.board.turn)
        self.board.push_uci(uci)
        print(self.board)

    def play_game(self, human_white=True, skill=0, pause=0.1):
        self.setup_game()
        if human_white == False:
            self.board.push_uci(self.stockfish_player())
            print(self.board)
        self.board.push_uci(self.human_player())
        print(self.board)
        '''
        while not self.board.is_game_over(claim_draw=True):
            if self.board.turn == chess.WHITE:
                uci = self.player1()
            else:
                uci = self.player2()
            #name = self.who(self.board.turn)
            self.board.push_uci(uci)
            print(self.board)
            print("\n")
        result, msg = self.get_result()
        print(msg)
        return (result, msg, self.board)
        '''
