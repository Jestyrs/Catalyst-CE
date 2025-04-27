import React from 'react';

// Placeholder components/icons - Replace with actual implementations
// Apply consistent styling
const UserAvatar = ({ online, username }: { online?: boolean; username?: string }) => (
  // Corrected className syntax
  <div className={`relative w-8 h-8 rounded-full flex items-center justify-center text-sm font-semibold ${username ? 'bg-purple-600 text-white' : 'bg-zinc-700 text-gray-400'}`}> 
    {username ? username.charAt(0).toUpperCase() : '?'}
    {online && (
      <span className="absolute bottom-0 right-0 block h-2.5 w-2.5 rounded-full bg-green-500 ring-2 ring-zinc-800"></span>
    )}
  </div>
);

const AddFriendIcon = () => <div className="text-gray-400 hover:text-gray-100 cursor-pointer">[+]</div>; // Basic placeholder
const CreateGroupIcon = () => <div className="text-gray-400 hover:text-gray-100 cursor-pointer">[G+]</div>; // Basic placeholder

// Friend List Item Component (Example)
interface FriendListItemProps {
  username: string;
  status: string; // e.g., 'Online', 'Idle', 'Offline', 'In Game'
  isOnline: boolean;
}

const FriendListItem: React.FC<FriendListItemProps> = ({ username, status, isOnline }) => (
  <div className="flex items-center space-x-3 p-2 rounded hover:bg-zinc-700 cursor-pointer"> {/* Updated hover */}
    <UserAvatar online={isOnline} username={username} />
    <div className="flex-grow">
      <div className="text-sm font-medium text-gray-200">{username}</div> {/* Updated text */}
      <div className="text-xs text-gray-400">{status}</div> {/* Updated text */}
    </div>
  </div>
);

// --- Component Props Definition ---
interface RightSidebarProps {
  isAuthenticated: boolean; // Use Auth0's state
  user: any; // Use Auth0's user object (type can be refined)
  onLogout: () => void; // Use the logout handler passed down
}

// --- Right Sidebar Component ---
const RightSidebar: React.FC<RightSidebarProps> = ({ isAuthenticated, user, onLogout }) => {
  // Dummy friends data - replace with prop
  const dummyFriends: FriendListItemProps[] = [
    { username: 'ShadowNinja', status: 'In Game - Valorant', isOnline: true }, 
    { username: 'PixelQueen', status: 'Online', isOnline: true },
    { username: 'CodeWizard', status: 'Idle', isOnline: true },
    { username: 'GhostRider', status: 'Offline', isOnline: false },
  ];

  return (
    // Updated root div styles
    <div className="h-full w-full flex flex-col bg-zinc-800 text-gray-300 border-l border-zinc-700 p-4 space-y-4 overflow-y-auto">

      {/* User Profile Section - Conditional */}
      {isAuthenticated && user && (
        <div className="mt-auto p-3 border-t border-zinc-700 flex items-center justify-between">
          <div className="flex items-center space-x-2">
            <UserAvatar username={user.nickname || user.name || user.email} online /> 
            <div>
              <div className="text-sm font-medium text-gray-100">{user.nickname || user.name || 'User'}</div>
              <div className="text-xs text-gray-400 truncate max-w-[100px]" title={user.email}>{user.email}</div> 
            </div>
          </div>
          <button 
            onClick={onLogout} // Use the passed onLogout prop
            className="bg-red-600 hover:bg-red-700 text-white text-xs font-bold py-1 px-3 rounded transition duration-150 ease-in-out"
            title="Logout"
          >
            Logout
          </button>
        </div>
      )}

      {!isAuthenticated && (
        <div className="mt-auto p-3 border-t border-zinc-700 text-center">
          <p className="text-xs text-gray-500">Login to see friends</p>
          {/* Optionally add a login button here if needed, although TopBar handles it */}
        </div>
      )}

      {/* Top Actions - Placeholder Icons */}
      <div className="flex justify-around pt-2 pb-2 border-b border-zinc-700"> {/* Updated border */}
        <AddFriendIcon />
        <CreateGroupIcon />
      </div>

      {/* Friends/Groups Toggle - Placeholder */}
      <div className="flex space-x-1 p-1 bg-zinc-900 rounded-md"> {/* Updated toggle bg */}
        <button className="flex-1 py-1 px-2 rounded text-xs font-medium bg-zinc-700 text-white focus:outline-none"> {/* Example active */}
          Friends
        </button>
        <button className="flex-1 py-1 px-2 rounded text-xs font-medium text-gray-400 hover:bg-zinc-700 hover:text-white focus:outline-none"> {/* Example inactive */}
          Chats
        </button>
      </div>

      {/* Friends List Section */}
      <div className="flex-grow overflow-y-auto space-y-1 -mr-2 pr-2"> {/* Negative margin trick for scrollbar spacing */}
        <h3 className="text-xs font-semibold uppercase text-gray-400 mb-2">Online - {dummyFriends.filter(f => f.isOnline).length}</h3>
        {dummyFriends.filter(f => f.isOnline).map(friend => (
          <FriendListItem key={friend.username} username={friend.username} status={friend.status} isOnline={friend.isOnline} />
        ))}

        <h3 className="text-xs font-semibold uppercase text-gray-400 pt-4 mb-2">Offline - {dummyFriends.filter(f => !f.isOnline).length}</h3>
        {dummyFriends.filter(f => !f.isOnline).map(friend => (
          <FriendListItem key={friend.username} username={friend.username} status={friend.status} isOnline={friend.isOnline} />
        ))}
      </div>

    </div>
  );
};

export default RightSidebar;
