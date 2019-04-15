using System;

namespace Iridinite.Wirefox.Battleships {

    /// <summary>
    /// Represents a single ship on a Battleships game board.
    /// </summary>
    public class Ship {

        /// <summary>
        /// Gets the kind of ship this instance represents.
        /// </summary>
        public ShipType Type { get; }

        /// <summary>
        /// Gets or sets how this ship is rotation on the board.
        /// </summary>
        public ShipOrientation Orientation { get; set; }

        /// <summary>
        /// Gets or sets this ship's position on the board.
        /// </summary>
        public BoardPosition Position { get; set; }

        /// <summary>
        /// Gets an array that represents this ship's damage state. Indices up to GetShipLength() are relevant.
        /// True indicates this index is hit, False indicates this index is still intact.
        /// </summary>
        public bool[] Damage { get; } = new bool[5];

        public Ship(ShipType type) {
            this.Type = type;
        }

        /// <summary>
        /// Returns the length of this ship on the board, in cells.
        /// </summary>
        public int GetShipLength() {
            switch (Type) {
                case ShipType.Carrier:
                    return 5;
                case ShipType.Battleship:
                    return 4;
                case ShipType.Cruiser:
                case ShipType.Submarine:
                    return 3;
                case ShipType.Destroyer:
                    return 2;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        /// <summary>
        /// Returns a value indicating whether this ship overlaps with a given BoardPosition.
        /// </summary>
        /// <param name="shot">The position to check for overlap with this ship.</param>
        /// <param name="index">If overlap, the relevant index into this ship's Damage array will be output here.</param>
        /// <returns></returns>
        public bool Overlaps(BoardPosition shot, out int index) {
            index = -1;

            if (Orientation == ShipOrientation.Horizontal) {
                if (shot.y != Position.y) return false; // is on the same line?
                index = shot.x - Position.x;
            } else {
                if (shot.x != Position.x) return false; // is on the same line?
                index = shot.y - Position.y;
            }

            // too short or too far?
            return index >= 0 && index < GetShipLength();
        }

        /// <summary>
        /// Process an incoming shot at the given position.
        /// </summary>
        /// <param name="shot">The cell that is under fire.</param>
        /// <returns>True if this ship has been hit, false if not.</returns>
        public bool DoIncomingShotAt(BoardPosition shot) {
            if (!Overlaps(shot, out var index)) return false;

            // this shot is within the ship's bounds! we got hit!
            Damage[index] = true;
            return true;
        }

        /// <summary>
        /// Returns a value indicating whether this ship has had all its cells hit.
        /// </summary>
        public bool IsSunk() {
            for (int i = 0; i < GetShipLength(); i++)
                if (!Damage[i])
                    return false;

            return true;
        }

    }

}
