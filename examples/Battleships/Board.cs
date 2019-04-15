using System.Collections.Generic;
using System.Linq;

namespace Iridinite.Wirefox.Battleships {

    /// <summary>
    /// Represents one player's Battleships game board.
    /// </summary>
    public class Board {

        /// <summary>
        /// The size of the game board, in cells, in both dimensions.
        /// </summary>
        public const int Dimensions = 10;

        /// <summary>
        /// Gets a collection representing all the ships that have been placed (so far).
        /// </summary>
        public Dictionary<ShipType, Ship> Ships { get; } = new Dictionary<ShipType, Ship>();

        /// <summary>
        /// Gets a collection representing all player shots fired on this board so far.
        /// </summary>
        public Dictionary<BoardPosition, ShotResult> Shots { get; } = new Dictionary<BoardPosition, ShotResult>();

        /// <summary>
        /// Places a new ship on the board, or changes an existing ship.
        /// </summary>
        /// <param name="type">The type of ship to place (unique identifier).</param>
        /// <param name="data">Metadata pertaining to this ship.</param>
        public void PlaceShip(ShipType type, Ship data) {
            if (Ships.ContainsKey(type))
                Ships[type] = data;
            else
                Ships.Add(type, data);
        }

        public void RemoveShip(ShipType type) {
            if (Ships.ContainsKey(type))
                Ships.Remove(type);
        }

        /// <summary>
        /// Fire a shot at a specific cell.
        /// </summary>
        /// <param name="shot">The location on this board where the shot is placed.</param>
        /// <returns>True if a ship was hit, false if the shot missed.</returns>
        public ShotResult DoIncomingShotAt(BoardPosition shot, out ShipType hitWhat) {
            hitWhat = 0;

            foreach (var ship in Ships.Values)
                if (ship.DoIncomingShotAt(shot)) {
                    hitWhat = ship.Type;
                    return ShotResult.Hit;
                }

            return ShotResult.Miss;
        }

        /// <summary>
        /// Records a shot result on a specific cell.
        /// </summary>
        /// <param name="shot">The location on this board where the shot is placed.</param>
        /// <param name="result">Whether or not this shot hit a ship.</param>
        public void SetShotResultAt(BoardPosition shot, ShotResult result) {
            if (Shots.ContainsKey(shot))
                Shots[shot] = result;
            else
                Shots.Add(shot, result);
        }

        /// <summary>
        /// Returns a value indicating whether the specified cell is free (does not overlap with a ship).
        /// </summary>
        /// <param name="pos">The cell to check.</param>
        public bool IsCellOccupied(BoardPosition pos) {
            return Ships.Any(pair => pair.Value.Overlaps(pos, out var index));
        }

        /// <summary>
        /// Returns a value indicating whether the given Ship can be placed at its current Position and Orientation.
        /// </summary>
        /// <param name="ship">The ship that is to be placed.</param>
        public bool CanPlaceShip(Ship ship) {
            // within board?
            var min = ship.Position;
            var max = ship.Position;
            if (ship.Orientation == ShipOrientation.Horizontal)
                max.x += ship.GetShipLength() - 1;
            else
                max.y += ship.GetShipLength() - 1;

            if (!min.IsWithinBoard()) return false;
            if (!max.IsWithinBoard()) return false;

            // no overlap with other ships?
            for (int x = min.x; x <= max.x; x++)
            for (int y = min.y; y <= max.y; y++)
                if (IsCellOccupied(new BoardPosition(x, y)))
                    return false;

            return true;
        }

        /// <summary>
        /// Returns a value indicating whether all ships on this board have been sunk.
        /// </summary>
        public bool GetAllShipsSunk() {
            return Ships.All(pair => pair.Value.IsSunk());
        }

    }

}
