import React from 'react';

// --- Type Definitions (Copied from App.tsx for now) ---
interface Game {
  id: string;
  name: string;
  status: 'ReadyToPlay' | 'Downloading' | 'Installing' | 'UpdateRequired' | 'NotInstalled' | 'Verifying' | 'Repairing';
  imageUrl: string;
  description: string;
  version: string;
  progress?: number;
}

interface UserProfile {
  username: string;
  // Add other profile details
}

type AuthStatus =
  | { status: 'LoggedIn'; profile: UserProfile }
  | { status: 'LoggedOut' }
  | { status: 'LoggingIn' }
  | { status: 'LoggingOut' }
  | { status: 'Error'; error?: string; profile?: UserProfile | null };

// --- Child Components (Copied from App.tsx for now) ---

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

// Authentication Area Component
const AuthArea: React.FC<{
  authStatus: AuthStatus | null;
  onLogout: () => void;
  loginError: string | null;
}> = ({ authStatus, onLogout, loginError }) => {
  const logoutButtonClasses = "w-full bg-red-700 hover:bg-red-800 text-white font-bold py-2 px-4 rounded transition duration-150 ease-in-out text-sm";

  return (
    <div className="text-sm">
      {loginError && (
        <p className="text-red-400 bg-red-900 bg-opacity-50 border border-red-700 p-2 rounded mb-3 text-center text-xs">Error: {loginError}</p>
      )}
      {authStatus?.status === 'LoggedIn' && (
        <div className="flex flex-col items-center space-y-2">
          <p className="text-gray-300">Welcome, <span className="font-semibold text-white">{authStatus.profile.username}</span>!</p>
          <button
            onClick={onLogout}
            className={logoutButtonClasses}
          >
            Logout
          </button>
        </div>
      )}
      {authStatus?.status === 'LoggingIn' && <p className="text-gray-400 text-center">Logging in...</p>}
      {authStatus?.status === 'LoggingOut' && <p className="text-gray-400 text-center">Logging out...</p>}
      {authStatus?.status === 'Error' && authStatus.error && !loginError && (
          <p className="text-red-400 bg-red-900 bg-opacity-50 border border-red-700 p-2 rounded mb-3 text-center text-xs">Session Error: {authStatus.error}</p>
      )}
    </div>
  );
};

interface MainContentAreaProps {
  activeView: string;
  isLoadingGames: boolean;
  apiError: string | null;
  games: Game[];
  selectedGameId: string | null;
  setSelectedGameId: (id: string | null) => void;
  authStatus: AuthStatus | null;
  handleLogout: () => void;
  loginError: string | null;
  selectedGame: Game | null;
  handleGameAction: (gameId: string, action: string) => void;
}

const MainContentArea: React.FC<MainContentAreaProps> = ({
  activeView,
  isLoadingGames,
  apiError,
  games,
  selectedGameId,
  setSelectedGameId,
  authStatus,
  handleLogout,
  loginError,
  selectedGame,
  handleGameAction
}) => {
  return (
    <main className="flex-grow p-6 overflow-y-auto bg-zinc-900"> 
       {isLoadingGames ? (
          <div className="flex justify-center items-center h-full">
            <p className="text-xl text-gray-400">Loading Launcher Data...</p>
          </div>
        ) : apiError ? (
          <div className="flex justify-center items-center h-full p-10">
            <p className="text-xl text-red-400 bg-red-900 bg-opacity-50 p-4 rounded border border-red-700">Error: {apiError}</p>
          </div>
        ) : activeView === 'Games' ? (
          <div className="grid grid-cols-3 gap-6 h-full">
            <div className="col-span-1 flex flex-col h-full overflow-hidden">
              <h2 className="text-xl font-semibold mb-4 px-3 pt-3 text-gray-200">My Games ({games.length})</h2>
              <div className="flex-grow overflow-y-auto pr-2 space-y-1 no-scrollbar pl-3 pb-3">
                {games.length > 0 ? (
                  games.map(game => (
                    <GameItem
                      key={game.id}
                      game={game}
                      isSelected={selectedGameId === game.id}
                      onClick={() => setSelectedGameId(game.id)}
                    />
                  ))
                ) : (
                  <p className="text-gray-500 text-center py-10">No games found.</p>
                )}
              </div>
              <div className="mt-auto p-3 border-t border-zinc-700"> 
                <AuthArea
                  authStatus={authStatus}
                  onLogout={handleLogout}
                  loginError={loginError}
                />
              </div>
            </div>

            <div className="col-span-2 h-full overflow-hidden">
              <GameDetails game={selectedGame} onAction={handleGameAction} />
            </div>
          </div>
        ) : (
          <div className="flex justify-center items-center h-full">
            <p className="text-xl text-gray-400">{activeView} View</p>
          </div>
        )}
    </main>
  );
};

export default MainContentArea;
