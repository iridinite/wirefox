namespace Iridinite.Wirefox.Battleships {

    /// <summary>
    /// Represents a cell position on the Battleships game board.
    /// </summary>
    public struct BoardPosition {

        public int x;
        public int y;

        public BoardPosition(int x, int y) {
            this.x = x;
            this.y = y;
        }

        /// <summary>
        /// Returns a value indicating whether this position is valid.
        /// </summary>
        public bool IsWithinBoard() {
            return x >= 0 && y >= 0 && x < Board.Dimensions && y < Board.Dimensions;
        }

    }

}
