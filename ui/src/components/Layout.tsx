import React from 'react';
import TopBar from './TopBar.tsx';
import LeftSidebar from './LeftSidebar.tsx';
import RightSidebar from './RightSidebar.tsx';
import MainContentArea from './MainContentArea.tsx';
import LoginModal from './LoginModal.tsx';

// --- Type Definitions (Copied from App.tsx/MainContentArea.tsx for now) ---
interface Game {
  id: string;
  name: string;
  status: 'ReadyToPlay' | 'Downloading' | 'Installing' | 'UpdateRequired' | 'NotInstalled' | 'Verifying' | 'Repairing';
  imageUrl: string;
  description: string;
  version: string;
  progress?: number;
}

// --- Layout Component Props ---
interface LayoutProps {
  activeView: string;
  setActiveView: (view: string) => void;
  // Props for MainContentArea & Sidebars
  isLoading: boolean; // Combined loading state (Auth + Games)
  error: string | null; // Combined error state (API + Auth + Login)
  games: Game[];
  selectedGameId: string | null;
  setSelectedGameId: (id: string | null) => void;
  selectedGame: Game | null;
  handleGameAction: (gameId: string, action: string) => void;
  version: string | null; // Added version prop
  // --- Authentication Props ---
  isAuthenticated: boolean;
  user: any; // Auth0 user object (use specific type if needed)
  isLoadingAuth: boolean; // Specific loading state for auth actions
  onLoginSubmit: (username: string) => Promise<void>; // Updated signature
  onLoginWithGoogle: () => void; // For Google login button
  onLogout: () => void; // For logout button
  // Props for Login Modal
  isLoginModalOpen: boolean;
  onOpenLoginModal: () => void; // Renamed for clarity
  onCloseLoginModal: () => void;
  loginError: string | null; // Keep specific error for modal display
}

const Layout: React.FC<LayoutProps> = ({
  activeView,
  setActiveView,
  // Destructure props
  isLoading,
  error,
  games,
  selectedGameId,
  setSelectedGameId,
  selectedGame,
  handleGameAction,
  version,
  // Destructure Auth props
  isAuthenticated,
  user,
  // isLoadingAuth is destructured but not directly used in Layout itself, which is okay.
  // It might be needed by child components if passed down further.
  onLoginSubmit, // Renamed internally to avoid conflict if needed
  onLoginWithGoogle,
  onLogout,
  // Destructure Modal props
  isLoginModalOpen,
  onOpenLoginModal,
  onCloseLoginModal,
  loginError,
  isLoadingAuth
}) => {
  console.log('[Layout] Rendering. IsAuthenticated:', isAuthenticated);
  return (
    <div className="flex flex-col h-screen bg-zinc-900 text-gray-200">
      {/* Pass necessary props to TopBar */}
      <TopBar
        isAuthenticated={isAuthenticated}
        user={user}
        onOpenLoginModal={onOpenLoginModal} // Pass handler to open modal
        onLogout={onLogout}
      />

      {/* Main content area */}
      <div className="flex flex-1 overflow-hidden pt-12"> {/* pt-12 assumes TopBar height */}
        {/* Left Sidebar */}
        <div className="w-64 flex-shrink-0 bg-zinc-800 overflow-y-auto">
          <LeftSidebar activeView={activeView} setActiveView={setActiveView} />
        </div>

        {/* Center Content & Right Sidebar Container */}
        <div className="flex flex-1 overflow-hidden">
          {/* Main Content Area */}
          <div className="flex-grow overflow-auto bg-zinc-900 p-4">
            <MainContentArea
              activeView={activeView}
              isLoading={isLoading}
              error={error}
              games={games}
              selectedGameId={selectedGameId}
              setSelectedGameId={setSelectedGameId}
              selectedGame={selectedGame}
              handleGameAction={handleGameAction}
              version={version}
            />
          </div>

          {/* Right Sidebar */}
          <div className="w-80 flex-shrink-0 bg-zinc-800 p-4 overflow-y-auto">
            <RightSidebar
              isAuthenticated={isAuthenticated}
              user={user}
              onLogout={onLogout}
            />
          </div>
        </div>
      </div>

      {/* Login Modal */}
      <LoginModal
        isOpen={isLoginModalOpen}
        onClose={onCloseLoginModal}
        onSubmit={onLoginSubmit}
        onLoginWithGoogle={onLoginWithGoogle} // Pass the handler down
        error={loginError}
        authStatus={isLoadingAuth ? 'LoggingIn' : (loginError ? 'Error' : '')}
      />
    </div>
  );
};

export default Layout;
