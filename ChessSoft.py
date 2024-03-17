import os
import time
import asyncio
import random
import chess
import chess.engine
import subprocess

class ChessSoft:
    def __init__(self, ser):
        self.board = None
        self.menu = None
        self.player1 = None
        self.player2 = None
        self.depth = 0
        self.time = 0.0
        self.serial = ser
        self.treset = False

    def random_player(self):
        move = random.choice(list(self.board.legal_moves))
        return move.uci()

    async def stockfish_player_async(self, engine):
        result = await engine.play(self.board, chess.engine.Limit(time=self.time, depth=self.depth))
        return result.move.uci()

    async def stockfish_player(self):
        transport, engine = await chess.engine.popen_uci("/usr/games/stockfish")
        # Initiate Stockfish computation without waiting for it to finish
        stockfish_task = asyncio.create_task(self.stockfish_player_async(engine))

        # Immediately return control so that move sound and random fact can be played
        return stockfish_task, engine

    async def stockfish_player(self):
        transport, engine = await chess.engine.popen_uci("/usr/games/stockfish")
        result = await engine.play(self.board, chess.engine.Limit(
            time=self.time, depth=self.depth))
        uci = result.move.uci()
        if self.menu != None:
            self.menu.show_game_status("COMP: " + uci)
        await engine.quit()
        occupancy = self.get_occupancy()
        occupancy_str = "occupancy:" + occupancy + "\n"
        self.serial.write(occupancy_str.encode('utf8'))
        move = chess.Move.from_uci(uci)
        # Get the piece being moved
        moving_piece = self.board.piece_at(move.from_square)
        moving_piece_letter = moving_piece.symbol().lower() if moving_piece else ''

        # Determine if a piece is being taken and what piece it is
        taken_piece = self.board.piece_at(move.to_square)
        taken_piece_letter = moving_piece_letter
        if taken_piece != None:
            taken_piece_letter = taken_piece.symbol().lower()
            if taken_piece_letter == moving_piece_letter:
                taken_piece_letter = 'x'
        start_square = chess.square_name(move.from_square)
        end_square = chess.square_name(move.to_square)
        extended_move = f"{start_square}{end_square}{moving_piece_letter}{taken_piece_letter}"
        print("extended move: " + extended_move)

        command = "comp:" + extended_move + "\n"
        self.serial.write(command.encode('utf8'))
        while True:
            if self.serial.inWaiting():
                line=self.serial.readline()
                break
        ack = line.decode('utf8').strip()
        # TODO compare move and ack. Ack is generated through sensing on board
        print("uci: " + uci + ", ack: " + ack)
        return uci

    async def get_hint(self):
        transport, engine = await chess.engine.popen_uci("/usr/games/stockfish")
        info = await engine.analyse(self.board, chess.engine.Limit(time=0.1))
        print("Score:", info["score"], info["pv"][0])
        await engine.quit()
        return info["pv"][0].uci()

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

    def get_occupancy(self):
        occupancy = []
        for square in chess.SQUARES:
            if self.board.piece_at(square):
                occupancy.append(square)
        string_elements = [str(item) for item in occupancy]
        formatted_string = ":".join(string_elements)
        return formatted_string

    def get_check_info(self):
        result_string = ""

        if self.board.is_check():
            # Get the square of the king that is in check
            king_color = self.board.turn
            king_square = self.board.king(king_color)

            # Get squares of pieces giving the check
            checking_pieces = self.board.checkers()
            checking_squares = [chess.square_name(square) for square in checking_pieces]

            # Convert the king's square and checking squares to a single string
            result_string = chess.square_name(king_square) + ':' + ':'.join(checking_squares)

        return result_string

    async def get_move(self):
        legal_uci_moves = [move.uci() for move in self.board.legal_moves]
        occupancy = self.get_occupancy()
        occupancy_str = "occupancy:" + occupancy + "\n"
        self.serial.write(occupancy_str.encode('utf8'))
        legal_uci_moves = [move.uci() for move in self.board.legal_moves]
        legal_uci_moves = sorted(legal_uci_moves)
        for char in "abcdefgh":
            moves_with_prefix = [move for move in legal_uci_moves if move.startswith(char)]
            legal_moves = "legal:" + (":".join(moves_with_prefix)) + "\n"
            print(legal_moves)
            self.serial.write(legal_moves.encode('utf8'))

        hint = await self.get_hint()
        hint_str = "hint:" + hint + "\n"
        self.serial.write(hint_str.encode('utf8'))
        print(hint_str)
        check = self.get_check_info()
        if check != "":
            check_str = "check:" + check + "\n"
            self.serial.write(check_str.encode('utf8'))
            print(check_str)
        start_str = "start\n"
        self.serial.write(start_str.encode('utf8'))
        if self.menu != None:
            self.menu.show_game_status("USER: ?")
        while True:
            if self.serial.inWaiting():
                line=self.serial.readline()
                break
        uci = line.decode('utf8').strip()
        if uci == 'ffff': # special move from board indicating override
            # Get it from the LCD menu
            uci = self.menu.get_manual_override().strip()
            # TODO: check if the move is legal
            occupancy = self.get_occupancy()
            occupancy_str = "occupancy:" + occupancy + "\n"
            self.serial.write(occupancy_str.encode('utf8'))
            override_str = "override:" + uci + "\n"
            print(override_str)
            self.serial.write(override_str.encode('utf8'))
            while True:
                if self.serial.inWaiting():
                    line=self.serial.readline()
                    break
            uci = line.decode('utf8').strip()
        #if self.menu != None and self.treset != True:
        if self.menu != None:
            self.menu.show_game_status("USER: " + uci)
        try:
            chess.Move.from_uci(uci)
            if uci not in legal_uci_moves:
                uci = None
        except:
            uci = None
        if uci == None:
            print("Invalid move")
        return uci

    def requires_promotion(self, uci):
        if len(uci) == 4:  # Standard UCI move length
            from_square = chess.SQUARE_NAMES.index(uci[:2])
            to_square = chess.SQUARE_NAMES.index(uci[2:])
            moving_piece = self.board.piece_at(from_square)
            if moving_piece.piece_type == chess.PAWN:
                if (chess.square_rank(to_square) == 7 and moving_piece.color == chess.WHITE) or \
                   (chess.square_rank(to_square) == 0 and moving_piece.color == chess.BLACK):
                    return True
        return False

    async def generate_move_sound(self, start_square, end_square, squares_dir='sounds/squares', moves_dir='sounds/moves'):
        # File paths for the input square sound files
        start_file = os.path.join(squares_dir, f"{start_square}.wav")
        end_file = os.path.join(squares_dir, f"{end_square}.wav")

        # Output move sound file path
        move_sound_file = os.path.join(moves_dir, f"move_{start_square}_to_{end_square}.wav")

        # Command to concatenate the square sound files using ffmpeg
        command = [
            'ffmpeg', '-y', '-i', start_file, '-i', end_file,
            '-filter_complex', '[0:0][1:0]concat=n=2:v=0:a=1[out]',
            '-map', '[out]', move_sound_file
        ]

        # Execute the ffmpeg command using asyncio.subprocess
        process = await asyncio.create_subprocess_exec(*command, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE)
        await process.wait()  # Wait for the process to finish

        print(f"Generated move sound: {move_sound_file}")
        return move_sound_file

    async def play_move_sound(self, start_square, end_square, squares_dir='sounds/squares', moves_dir='sounds/moves'):
        move_sound_file = os.path.join(moves_dir, f"move_{start_square}_to_{end_square}.wav")

        if not os.path.exists(move_sound_file):
            print(f"Sound for move {start_square} to {end_square} not found. Generating now...")
            # Ensure generate_move_sound is also an async function if it involves subprocess or I/O operations
            await self.generate_move_sound(start_square, end_square, squares_dir, moves_dir)

        process = await asyncio.create_subprocess_exec('aplay', move_sound_file)
        await process.wait()  # Wait for the move sound to finish playing

    async def play_random_fact(self):
        sounds_dir = "sounds/facts"
        fact_files = [f for f in os.listdir(sounds_dir) if f.startswith('fact_') and f.endswith('.wav')]

        if fact_files:
            random_fact_file = random.choice(fact_files)
            fact_path = os.path.join(sounds_dir, random_fact_file)
            process = await asyncio.create_subprocess_exec('aplay', fact_path)
        else:
            print("No fact wav files found.")

    async def sequential_sound_play(self, start_square, end_square):
        await self.play_move_sound(start_square, end_square)
        await self.play_random_fact()

    async def human_player(self):
        uci = None
        while uci is None:
            uci = await self.get_move()
        if uci and self.requires_promotion(uci):
            uci += 'q'  # Append 'q' to promote to queen by default
        start_square, end_square = uci[:2], uci[2:4]
        await self.sequential_sound_play(start_square, end_square)
        return uci

    def who(self, player):
        return "whit" if player == chess.WHITE else "blac"

    def install_menu_ref(self, m):
        self.menu = m

    def update_skill(self, skill):
        if skill <= 0:
            self.depth = 22
            self.time = 1.0
        elif skill == 1:
            self.depth = 13
            self.time = 0.5
        elif skill == 2:
            self.depth = 8
            self.time = 0.4
        elif skill == 3:
            self.depth = 5
            self.time = 0.2
        elif skill == 4:
            self.depth = 5
            self.time = 0.1
        elif skill >= 5:
            self.depth = 5
            self.time = 0.05
        print("skill depth: " + str(self.depth) + ", skill time: " + str(self.time))

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
        self.treset = False

    def reset_target(self):
        self.treset = True

    def is_game_set(self):
        return self.board != None

    def game_over(self):
        return self.board.is_game_over(claim_draw=True)

    def show_result(self):
        if self.board.is_checkmate():
            occupancy = self.get_occupancy()
            occupancy_str = "occupancy:" + occupancy + "\n"
            self.serial.write(occupancy_str.encode('utf8'))
            king_square = chess.square_name(self.board.king(self.board.turn))
            result_str = "checkmate:" + king_square + ":" + self.who(not self.board.turn) + "\n"
            self.serial.write(result_str.encode('utf8'))
            self.menu.show_game_status(result_str)
            print(result_str)
            '''
            while True:
                if self.serial.inWaiting():
                    line=self.serial.readline()
                    break
            uci = line.decode('utf8').strip()
            if uci == 'f0f0': # special move from board indicating override
                self.board = None
            '''

    async def play_next_move(self):
        if self.treset:
            self.serial.write("reset\n".encode('utf8'))
            self.board = None
            return
        if self.board.turn == chess.WHITE:
            uci = await self.player1()
        else:
            uci = await self.player2()
        self.board.push_uci(uci)
        print(self.board)

    def is_human_turn(self):
        if self.board.turn == chess.WHITE and self.player1 == self.human_player:
            return True
        elif self.board.turn == chess.BLACK and self.player2 == self.human_player:
            return True
        return False

    def play_game(self, human_white=True, skill=0, pause=0.1):
        self.setup_game()
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
