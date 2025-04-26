import React from 'react';

// Placeholder components/icons - Replace with actual implementations
const UserAvatar = ({ online }: { online?: boolean }) => (
    <div className={`relative w-8 h-8 rounded-full bg-purple-500 ${online ? 'border-2 border-neutral-400' : 'border-2 border-neutral-500'}`}></div>
);
const AddFriendIcon = () => <div className="w-5 h-5 bg-blue-500 rounded-sm">+</div>;
const GroupIcon = () => <div className="w-5 h-5 bg-orange-500 rounded-sm">G</div>;

// Placeholder FriendListItem
interface FriendListItemProps {
  id: string;
  username: string;
  statusText: string;
  isOnline: boolean;
}
const FriendListItem: React.FC<FriendListItemProps> = ({ id, username, statusText, isOnline }) => (
  <div key={id} className="flex items-center space-x-2 p-1 hover:bg-neutral-700 rounded cursor-pointer">
    <UserAvatar online={isOnline} />
    <div className="flex-grow">
      <div className="text-sm text-neutral-100">{username}</div>
      <div className="text-xs text-neutral-400 truncate">{statusText}</div>
    </div>
  </div>
);

interface RightSidebarProps {
  // Add props for friends list data, event handlers etc.
  // friends: Friend[];
}

const RightSidebar: React.FC<RightSidebarProps> = (/* { friends } */) => {

  // Dummy friends data - replace with prop
  const dummyFriends: FriendListItemProps[] = [
    { id: 'friend1', username: 'ShadowGamer', statusText: 'Playing Stellar Conquest', isOnline: true },
    { id: 'friend2', username: 'MysticWizard', statusText: 'Online', isOnline: true },
    { id: 'friend3', username: 'CyberNinja', statusText: 'Offline', isOnline: false },
    { id: 'friend4', username: 'CosmicVoyager', statusText: 'In Launcher', isOnline: true },
  ];

  return (
    <div className="h-full w-full flex flex-col bg-neutral-800 text-neutral-300 border-l border-neutral-700">
      {/* Top Actions */}
      <div className="flex justify-around p-3 border-b border-neutral-700">
        <AddFriendIcon />
        <GroupIcon />
        {/* Add other action icons as needed */}
      </div>

      {/* Friends List */}
      <div className="flex-grow overflow-y-auto p-2 no-scrollbar">
        <div className="flex justify-between items-center text-sm mb-2 px-1">
          <span className="font-semibold text-neutral-100">Friends</span>
          <span className="text-neutral-400">{dummyFriends.filter(f => f.isOnline).length} Online</span>
        </div>
        <div className="space-y-1">
          {dummyFriends.map(friend => (
            <FriendListItem
              key={friend.id}
              id={friend.id}
              username={friend.username}
              statusText={friend.statusText}
              isOnline={friend.isOnline}
            />
          ))}
        </div>
      </div>

      {/* Chats/Groups Button */}
      <div className="p-3 text-center bg-neutral-900 hover:bg-neutral-700 cursor-pointer border-t border-neutral-700">
        Chats & Groups
      </div>
    </div>
  );
};

export default RightSidebar;
