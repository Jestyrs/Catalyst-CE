import React from 'react';

// Placeholder NavItem Component - needs state handling for 'active'
interface NavItemProps {
  icon: React.ReactNode;
  label: string;
  isActive?: boolean;
  onClick: () => void;
}
const NavItem: React.FC<NavItemProps> = ({ icon, label, isActive, onClick }) => {
  const baseClasses = "flex flex-col items-center justify-center w-16 h-16 rounded cursor-pointer";
  const activeClasses = isActive ? "bg-blue-600 text-white" : "text-gray-400 hover:bg-gray-700 hover:text-white";
  return (
    <div className={`${baseClasses} ${activeClasses}`} onClick={onClick}>
      <div className="w-6 h-6 mb-1">{icon}</div>
      <span className="text-xs">{label}</span>
    </div>
  );
};

// Placeholder FavoriteGameIcon
const FavoriteGameIcon = ({ id }: { id: string }) => (
  <div key={id} className="w-12 h-12 rounded bg-gray-600 flex items-center justify-center text-xs cursor-pointer hover:bg-gray-500">
    {id.substring(0, 3)}
  </div>
);

// Placeholder AddFavoriteButton
const AddFavoriteButton = () => (
  <div className="w-12 h-12 rounded bg-gray-700 flex items-center justify-center text-gray-400 cursor-pointer hover:bg-gray-600">
    +
  </div>
);

interface LeftSidebarProps {
  activeView: string;
  setActiveView: (view: string) => void;
  // TODO: Add favorites: Game[]; prop later
}

const LeftSidebar: React.FC<LeftSidebarProps> = ({ activeView, setActiveView /*, favorites */ }) => {
  // Dummy favorites - replace with prop later
  const dummyFavorites = [{ id: 'game1' }, { id: 'game2' }];

  return (
    <div className="fixed left-0 top-0 bottom-0 flex flex-col justify-between h-screen pt-12 bg-gray-800 text-gray-400 w-20 items-center py-4 border-r border-gray-700">
      {/* Top Navigation Section */}
      <div className="flex flex-col space-y-4 items-center mt-2">
        {/* Replace placeholders with actual icons */}
        <NavItem icon={<span>🏠</span>} label="Home" onClick={() => setActiveView('Home')} isActive={activeView === 'Home'} />
        <NavItem icon={<span>🎮</span>} label="Games" onClick={() => setActiveView('Games')} isActive={activeView === 'Games'} />
        <NavItem icon={<span>🛒</span>} label="Shop" onClick={() => setActiveView('Shop')} isActive={activeView === 'Shop'} />
        <NavItem icon={<span>🛡️</span>} label="Clans" onClick={() => setActiveView('Clans')} isActive={activeView === 'Clans'} />
      </div>

      {/* Favorites Section */}
      <div className="flex-grow overflow-y-auto mt-6 w-full flex flex-col items-center space-y-2 no-scrollbar">
        <span className="text-xs uppercase text-gray-500 mb-2">Favorites</span>
        {dummyFavorites.map(fav => (
          <FavoriteGameIcon key={fav.id} id={fav.id} />
        ))}
        <AddFavoriteButton />
      </div>

      {/* Optional Bottom Section (if needed) */}
      <div>
        {/* Settings or other icons could go here */}
      </div>
    </div>
  );
};

export default LeftSidebar;
