import os
import time
import asyncio
import random
import chess
import chess.engine
import subprocess
import logging

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
        self.snore = False
        self.play_capture = True
        self.prev_comments = []
        log_directory = "/home/pi/ChessMate/logs"
        os.makedirs(log_directory, exist_ok=True)
        log_filename = os.path.join(log_directory, "chesssoft.log")
        self.chesssoft_logger = self.setup_custom_logger('ChessSoftLogger', log_filename)

    def setup_custom_logger(self, name, log_file, level=logging.DEBUG):
        """Set up a custom logger that writes to a specific file."""
        logger = logging.getLogger(name)
        logger.setLevel(level)
        file_handler = logging.FileHandler(log_file)
        file_handler.setLevel(level)
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
        file_handler.setFormatter(formatter)
        logger.addHandler(file_handler)
        return logger

    def random_player(self):
        move = random.choice(list(self.board.legal_moves))
        return move.uci()

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
            if self.play_capture:
                await self.play_capture_sound(taken_piece_letter)
            self.play_capture = True
            if taken_piece_letter == moving_piece_letter:
                taken_piece_letter = 'x'
        start_square = chess.square_name(move.from_square)
        end_square = chess.square_name(move.to_square)
        extended_move = f"{start_square}{end_square}{moving_piece_letter}{taken_piece_letter}"
        print("extended move: " + extended_move)
        self.chesssoft_logger.info("extended move: " + extended_move)

        command = "comp:" + extended_move + "\n"
        self.serial.write(command.encode('utf8'))
        while True:
            if self.serial.inWaiting():
                line=self.serial.readline()
                break
        ack = line.decode('utf8').strip()
        # TODO compare move and ack. Ack is generated through sensing on board
        print("uci: " + uci + ", ack: " + ack)
        self.chesssoft_logger.info("uci: " + uci + ", ack: " + ack)
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
        self.chesssoft_logger.info("Legal moves: " + (",".join(sorted(legal_uci_moves))))
        possible_moves = list(filter(lambda x: start in x, legal_uci_moves))
        print("Possible moves: " + str(possible_moves))
        self.chesssoft_logger.info("Possible moves: " + str(possible_moves))
        if color != self.board.turn or color == None or not possible_moves:
            #print("Invalid move")
            return None
        possible_squares = [sub.replace(start, '') for sub in possible_moves]
        print("Possible squares: " + str(possible_squares))
        self.chesssoft_logger.info("Possible squares: " + str(possible_squares))
        for sq in possible_squares:
            print(chess.parse_square(sq))
            self.chesssoft_logger.info(chess.parse_square(sq))
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
        start_time = time.time()
        move_timeout = 20  # seconds
        hint_timeout_reached = False

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
            self.chesssoft_logger.info(legal_moves)
            self.serial.write(legal_moves.encode('utf8'))

        hint = await self.get_hint()
        hint_str = "hint:" + hint + "\n"
        self.serial.write(hint_str.encode('utf8'))
        print(hint_str)
        self.chesssoft_logger.info(hint_str)
        check = self.get_check_info()
        if check != "":
            check_str = "check:" + check + "\n"
            self.serial.write(check_str.encode('utf8'))
            await self.play_comment("comment_3.wav")
            print(check_str)
            self.chesssoft_logger.info(check_str)
        start_str = "start\n"
        self.serial.write(start_str.encode('utf8'))
        if self.menu != None:
            self.menu.show_game_status("USER: ?")
        while True:
            if self.serial.inWaiting():
                line=self.serial.readline()
                uci = line.decode('utf8').strip()
                break
            elif not hint_timeout_reached and self.snore and time.time() - start_time > move_timeout:
                asyncio.create_task(self.play_comment("snore.wav"))
                hint_timeout_reached = True
            await asyncio.sleep(0.1)  # Add a small sleep to prevent a tight loop

        if uci == 'ffff': # special move from board indicating override
            # Get it from the LCD menu
            uci = self.menu.get_manual_override().strip()
            # TODO: check if the move is legal
            occupancy = self.get_occupancy()
            occupancy_str = "occupancy:" + occupancy + "\n"
            self.serial.write(occupancy_str.encode('utf8'))
            override_str = "override:" + uci + "\n"
            print(override_str)
            self.chesssoft_logger.info(override_str)
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
            self.chesssoft_logger.info("Invalid move")
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

    async def play_capture_sound(self, piece_letter):
        piece_sound_map = {
            'k': 'king_0.wav',
            'q': 'queen_0.wav',
            'b': 'bishop_0.wav',
            'n': 'knight_0.wav',
            'r': 'rook_0.wav',
            'p': 'pawn_0.wav'
        }
        sound_file = piece_sound_map.get(piece_letter.lower(), None)
        if sound_file:
            sound_path = os.path.join("sounds/pieces", sound_file)
            if os.path.exists(sound_path):
                process = await asyncio.create_subprocess_exec('aplay', sound_path)
                await process.wait()
            else:
                print(f"Sound file for {piece_letter} not found at {sound_path}")
        else:
            print(f"No sound mapping found for piece letter: {piece_letter}")

    async def play_move_sound(self, start_square, end_square, squares_dir='sounds/squares', moves_dir='sounds/moves'):
        move_sound_file_path = os.path.join(moves_dir, start_square, f"move_{start_square}_to_{end_square}.wav")
        if not os.path.exists(move_sound_file_path):
            print(f"Sound for move {start_square} to {end_square} not found in {start_square} folder. Generating now...")
            await self.generate_move_sound(start_square, end_square, squares_dir, moves_dir)
        process = await asyncio.create_subprocess_exec('aplay', move_sound_file_path)

        await process.wait()  # Wait for the move sound to finish playing

    def play_comment_sync(self, comment_file):
        comments_dir = "sounds/comments"
        comment_sound_file = os.path.join(comments_dir, comment_file)

        # Use subprocess.run to play the comment file synchronously
        subprocess.run(['aplay', comment_sound_file], check=True)

    async def play_comment(self, comment_file):
        comments_dir = "sounds/comments"
        comment_sound_file = os.path.join(comments_dir, comment_file)
        process = await asyncio.create_subprocess_exec('aplay', comment_sound_file)
        await process.wait()  # Wait for the move sound to finish playing

    async def play_random_fact(self):
        facts_dir = "sounds/facts"
        fact_files = [f for f in os.listdir(facts_dir) if f.startswith('fact_') and f.endswith('.wav')]

        if fact_files:
            random_fact_file = random.choice(fact_files)
            fact_path = os.path.join(facts_dir, random_fact_file)
            process = await asyncio.create_subprocess_exec('aplay', fact_path)
        else:
            print("No fact wav files found.")

    async def play_comment_or_fact_based_on_history(self, comment_file):
        # Check if the last two comments were comment_0.wav or comment_1.wav
        if self.prev_comments[-2:] == ["comment_0.wav", "comment_0.wav"] or \
           self.prev_comments[-2:] == ["comment_1.wav", "comment_1.wav"] or \
           self.prev_comments[-2:] == ["comment_0.wav", "comment_1.wav"] or \
           self.prev_comments[-2:] == ["comment_1.wav", "comment_0.wav"]:
            # Play a random fact
            self.play_capture = False
            await self.play_random_fact()
            # Reset the history
            self.prev_comments = []
        else:
            # Play the comment and update the history
            await self.play_comment(comment_file)
            self.prev_comments.append(comment_file)

    async def sequential_sound_play(self, start_square, end_square, score):
        await self.play_move_sound(start_square, end_square)
        if score >= 1 and score <= 2:
            await self.play_comment_or_fact_based_on_history("comment_0.wav")
        elif score >= 3 and score <= 4:
            await self.play_comment_or_fact_based_on_history("comment_1.wav")
        else:
            # Always play a random fact if the score is 5, also reset history
            self.play_capture = False
            await self.play_random_fact()
            self.prev_comments = []

    async def human_player(self):
        uci = None
        while uci is None:
            uci = await self.get_move()
        if uci and self.requires_promotion(uci):
            uci += 'q'  # Append 'q' to promote to queen by default
        start_square, end_square = uci[:2], uci[2:4]
        score = await self.evaluate_move(chess.Move.from_uci(uci))
        print("Score: " + str(score))
        self.chesssoft_logger.info("Score: " + str(score))
        await self.sequential_sound_play(start_square, end_square, score)
        return uci

    async def evaluate_move(self, move: chess.Move) -> int:
        """
        Returns a score from 1 to 5, with 1 being the best move.
        """
        transport, engine = await chess.engine.popen_uci("/usr/games/stockfish")
        info = await engine.analyse(self.board, chess.engine.Limit(time=0.1))
        pv = info.get("pv")  # Principal variation (best move sequence)
        if pv is None or len(pv) == 0:
            await engine.quit()
            return 5  # Cannot evaluate, so assume the worst score
        # Generate scores based on the move's ranking within the engine's suggestions
        try:
            move_rank = pv.index(move) + 1
        except ValueError:
            move_rank = len(pv) + 1  # Move was not in the engine's top suggestions
        await engine.quit()
        # Convert move_rank to a score from 1 to 5
        score = min(5, move_rank)
        return score

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
        self.chesssoft_logger.info("skill depth: " + str(self.depth) + ", skill time: " + str(self.time))

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
        self.snore = False

    def setup_puzzle(self, human_white=True, skill=0, pause=0.1):
        self.update_skill(skill)
        self.update_players(human_white)
        self.board = chess.Board("r2qk2r/pb4pp/1n2Pb2/2B2Q2/p1p5/2P5/2B2PPP/RN2R1K1 w - - 1 0")
        self.treset = False
        self.snore = False
        self.play_comment_sync("comment_4.wav")

    def reset_board(self):
        self.board = None

    def reset_target(self):
        self.treset = True

    def is_game_set(self):
        return self.board != None

    def game_over(self):
        return self.board.is_game_over(claim_draw=True)

    async def show_result(self):
        if self.board.is_checkmate():
            occupancy = self.get_occupancy()
            occupancy_str = "occupancy:" + occupancy + "\n"
            self.serial.write(occupancy_str.encode('utf8'))
            king_square = chess.square_name(self.board.king(self.board.turn))
            result_str = "checkmate:" + king_square + ":" + self.who(not self.board.turn) + "\n"
            self.serial.write(result_str.encode('utf8'))
            self.menu.show_game_status(result_str)
            print(result_str)
            self.chesssoft_logger.info(result_str)
            await self.play_comment("comment_2.wav")
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
        self.snore = True
        self.board.push_uci(uci)
        print(self.board)
        self.chesssoft_logger.info(self.board)

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
            self.chesssoft_logger.info(self.board)
            print("\n")
            self.chesssoft_logger.info("\n")
        result, msg = self.get_result()
        print(msg)
        self.chesssoft_logger.info(msg)
        return (result, msg, self.board)
