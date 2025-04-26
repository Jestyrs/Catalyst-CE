// --- Type Definitions ---

export interface Game {
  id: string;
  name: string;
  status: 'ReadyToPlay' | 'Downloading' | 'Installing' | 'UpdateRequired' | 'NotInstalled' | 'Verifying' | 'Repairing';
  imageUrl: string;
  description: string;
  version: string;
  progress?: number; // Optional download/install progress (0-100)
}

export interface UserProfile {
  username: string;
  // Add other profile details: avatarUrl, email, etc.
}

export type AuthStatus =
  | { status: 'LoggedIn'; profile: UserProfile }
  | { status: 'LoggedOut' }
  | { status: 'LoggingIn' }
  | { status: 'LoggingOut' }
  | { status: 'Error'; error?: string; profile?: UserProfile | null }; // Keep profile on error if possible

// Represents potential errors from the C++ backend
export interface GameLauncherAPIError {
  errorCode: number;
  errorMessage: string;
}

// Represents game update events (e.g., download progress)
export interface GameUpdateEvent {
  gameId: string;
  status: Game['status']; // Reuse status from Game type
  progress?: number;
  error?: GameLauncherAPIError;
}

// --- C++ Injected API Structure ---

// Define the structure of the API injected into the window object
export interface GameLauncherAPI {
  // Asynchronous methods using callbacks
  getGameList: (onSuccess: (gamesJson: string) => void, onFailure: (errorCode: number, errorMessage: string) => void) => void;
  getAuthStatus: (onSuccess: (statusJson: string) => void, onFailure: (errorCode: number, errorMessage: string) => void) => void;
  getVersion: (onSuccess: (version: string) => void, onFailure: (errorCode: number, errorMessage: string) => void) => void;
  performGameAction: (action: 'install' | 'launch' | 'update' | 'verify' | 'repair' | 'cancel', gameId: string, onSuccess: () => void, onFailure: (errorCode: number, errorMessage: string) => void) => void;
  login: (username: string, password: string, onSuccess: (statusJson: string) => void, onFailure: (errorCode: number, errorMessage: string) => void) => void;
  logout: (onSuccess: (statusJson: string) => void, onFailure: (errorCode: number, errorMessage: string) => void) => void;

  // Synchronous methods (if any - less common for potentially long operations)
  // Example: getSyncData?: () => string;

  // Event subscription (Optional but recommended for progress updates)
  // Example: subscribeToGameUpdates?: (callback: (event: GameUpdateEvent) => void) => string; // Returns subscription ID
  // Example: unsubscribeFromGameUpdates?: (subscriptionId: string) => void;
}

// --- CEF Query Function Structure (If using cefQuery) ---

// Define the structure for cefQuery requests
export interface CefQueryRequest {
  request: string; // JSON string payload
  persistent?: boolean;
  onSuccess: (response: string) => void; // JSON string response
  onFailure: (error_code: number, error_message: string) => void;
}

// --- Global Window Declaration ---

// Extend the Window interface to declare the injected objects
declare global {
  interface Window {
    gameLauncherAPI?: GameLauncherAPI;
    // Define cefQuery if you are using it directly for communication
    cefQuery?: (query: CefQueryRequest) => void;
    // Optional: Define cefQueryCancel if needed
    cefQueryCancel?: (queryId: number) => void; // Assuming queryId is a number
    // Add listener functions if C++ injects them directly
    registerGameUpdateListener?: (listener: (eventJson: string) => void) => void;
    unregisterGameUpdateListener?: (listener: (eventJson: string) => void) => void;
  }
}
