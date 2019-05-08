namespace Iridinite.Wirefox.Battleships {

    /// <summary>
    /// Represents a custom packet command related to Battleships gameplay.
    /// </summary>
    internal enum GameCommand : byte {
        BeginSession = PacketCommand.USER_PACKET,
        Chat,

        PlaceShip,

        BeginGame,
        YourTurn,
        OpponentTurn,
        Shoot,
        ShootResult,
        ShootIncoming,

        GameWin,
        GameLose,
        OpponentDisconnected
    }

}
