import React from 'react';

// --- Type Definitions ---
interface Game {
  id: string;
  name: string;
  status: 'ReadyToPlay' | 'Downloading' | 'Installing' | 'UpdateRequired' | 'NotInstalled' | 'Verifying' | 'Repairing';
  imageUrl: string;
  description: string;
  version: string;
  progress?: number;
}

// --- Child Components ---

// Game Item Component
const GameItem: React.FC<{ game: Game; isSelected: boolean; onClick: () => void }> = React.memo(({ game, isSelected, onClick }) => {
  return (
    <div
      className={`flex items-center p-3 rounded-lg mb-2 cursor-pointer transition-colors duration-150 ease-in-out \
                  ${isSelected ? 'bg-purple-600 shadow-md text-white' : 'bg-zinc-800 hover:bg-zinc-700 text-gray-300'}`}
      onClick={onClick}
    >
      <img src={game.imageUrl} alt={game.name} className="w-16 h-10 object-cover rounded mr-4 flex-shrink-0" />
      <div className="flex-grow">
        <h3 className={`font-semibold ${isSelected ? 'text-white' : 'text-gray-100'}`}>{game.name}</h3>
        <p className={`text-xs ${isSelected ? 'text-purple-200' : 'text-gray-400'}`}>{game.status}</p>
      </div>
    </div>
  );
});

// Game Details Component
const GameDetails: React.FC<{ game: Game | null; onAction: (gameId: string, action: string) => void }> = ({ game, onAction }) => {
  if (!game) {
    return <div className="text-center text-gray-500 p-10">Select a game to see details</div>;
  }

  const renderActionButton = () => {
    const buttonBaseClasses = "w-full font-bold py-2.5 px-4 rounded transition duration-150 ease-in-out text-sm";
    const primaryButtonClasses = `bg-purple-600 hover:bg-purple-700 text-white ${buttonBaseClasses}`;
    const disabledButtonClasses = `bg-zinc-600 text-gray-400 cursor-not-allowed ${buttonBaseClasses}`;

    switch (game.status) {
      case 'ReadyToPlay':
        return <button onClick={() => onAction(game.id, 'launch')} className={primaryButtonClasses}>Play</button>;
      case 'UpdateRequired':
        return <button onClick={() => onAction(game.id, 'update')} className={primaryButtonClasses}>Update</button>;
      case 'NotInstalled':
        return <button onClick={() => onAction(game.id, 'install')} className={primaryButtonClasses}>Install</button>;
      case 'Downloading':
      case 'Installing':
      case 'Verifying':
      case 'Repairing':
        return (
          <div className="w-full bg-zinc-700 rounded h-9 flex items-center justify-center relative overflow-hidden">
            <div className="absolute left-0 top-0 h-full bg-purple-600 transition-all duration-300 ease-linear" style={{ width: `${game.progress || 0}%` }}></div>
            <span className="relative z-10 text-white text-xs font-medium">{game.status}... {game.progress || 0}%</span>
          </div>
        );
      default:
        return <button className={disabledButtonClasses} disabled>Unavailable</button>;
    }
  };

  return (
    <div className="flex flex-col h-full bg-zinc-800 rounded-lg overflow-hidden shadow-inner">
      <img src={game.imageUrl} alt={game.name} className="w-full h-48 object-cover" />
      <div className="p-6 flex-grow flex flex-col">
        <h2 className="text-2xl font-bold mb-2 text-gray-100">{game.name}</h2>
        <p className="text-gray-400 text-sm mb-4 flex-grow">{game.description}</p>
        <div className="flex justify-between items-center text-xs text-gray-500 mb-4">
          <span>Version: {game.version}</span>
          {/* Add other details like playtime, last played etc. */}
        </div>
        <div className="mt-auto">
          {renderActionButton()}
        </div>
      </div>
    </div>
  );
};

// --- Main Component Props ---
interface MainContentAreaProps {
  activeView: string;
  isLoading: boolean;
  error: string | null;
  games: Game[];
  selectedGameId: string | null;
  setSelectedGameId: (id: string | null) => void;
  selectedGame: Game | null;
  handleGameAction: (gameId: string, action: string) => void;
  version: string | null;
}

// --- Main Component ---
const MainContentArea: React.FC<MainContentAreaProps> = ({
  activeView,
  isLoading,
  error,
  games,
  selectedGameId,
  setSelectedGameId,
  selectedGame,
  handleGameAction
}) => {

  // --- Loading and Error States ---
  if (isLoading) {
    return <div className="p-10 text-center text-gray-400">Loading games...</div>;
  }

  if (error) {
    return (
      <div className="p-10 text-center text-red-400 bg-red-900/30 rounded-lg">
        <h2 className="text-xl font-semibold mb-2">Error</h2>
        <p>{error}</p>
      </div>
    );
  }

  // --- View Rendering ---
  const renderContent = () => {
    switch (activeView) {
      case 'Games':
        return (
          <div>
            <h2 className="text-xl font-semibold mb-4 text-gray-100">Library</h2>
            {games.length > 0 ? (
              games.map((game) => (
                <GameItem
                  key={game.id}
                  game={game}
                  isSelected={selectedGameId === game.id}
                  onClick={() => setSelectedGameId(game.id)}
                />
              ))
            ) : (
              <p className="text-gray-500">No games found in library.</p>
            )}
          </div>
        );
      case 'GameDetails':
        return <GameDetails game={selectedGame} onAction={handleGameAction} />;
      // Add cases for 'Settings', 'Downloads', etc.
      default:
        return <div className="p-10 text-center text-gray-500">Select a view</div>;
    }
  };

  return (
    <div className="p-4">
      {renderContent()}
      {/* Conditionally render version at bottom-right - might be better in Layout */}
      {/* {version && <div className="fixed bottom-1 right-1 text-xs text-gray-500">v{version}</div>} */}
    </div>
  );
};

export default MainContentArea;
