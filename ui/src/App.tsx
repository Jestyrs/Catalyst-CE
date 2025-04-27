import React, { useState, useEffect, useMemo, useCallback } from 'react';
import { useAuth0 } from '@auth0/auth0-react';

import Layout from './components/Layout.tsx';

// @ts-ignore - TS struggles to see GameUpdateEvent usage within window listener callback

import { Game, AuthStatus, GameLauncherAPI } from './api.ts';

// --- Main Application Component ---

const App: React.FC = () => {

  // --- Auth0 Hook ---
  const {
    loginWithRedirect,
    loginWithPopup,
    logout,
    user,
    isAuthenticated,
    isLoading: isLoadingAuth, // Renamed to avoid conflict with isLoadingGames
    error: authError // Auth0 specific error
  } = useAuth0();

  // --- State ---

  const [games, setGames] = useState<Game[]>([]);
  const [selectedGameId, setSelectedGameId] = useState<string | null>(null);
  const [isLoadingGames, setIsLoadingGames] = useState(true);
  const [loginError, setLoginError] = useState<string | null>(null); // Reinstated for username/password flow errors
  const [api, setApi] = useState<GameLauncherAPI | null>(null);
  const [apiError, setApiError] = useState<string | null>(null);
  const [activeView, setActiveView] = useState<string>('Games');
  const [isLoginModalOpen, setIsLoginModalOpen] = useState<boolean>(false);
  const [version, ] = useState<string | null>(null); 

  // --- API Initialization & Data Fetching Effects ---

  useEffect(() => {

    const checkForApi = async () => {

      // Give CEF potentially a bit more time

      await new Promise(resolve => setTimeout(resolve, 1500)); // Wait for potential injection



      if (window.gameLauncherAPI) {

        console.log("Real API found. Setting base API state.");

        const resolvedApi = window.gameLauncherAPI;

        setApi(resolvedApi); // Set the base API, triggers apiWrapper creation

      } else {

        console.error("CRITICAL: window.gameLauncherAPI not found after delay!");

        setApiError("Failed to connect to the launcher backend. Please restart the launcher.");

        // Keep loading games false so the error message shows

        setGames([]);

      }



      // Only set loading to false after attempting to fetch or determining API is missing

      setIsLoadingGames(false);

    };



    checkForApi();



    // No cleanup needed for API listener in this version



  }, []); // Empty dependency array ensures this runs once on mount



  // --- Helper Types (within App) ---

  // General purpose callback types (can be reused if needed elsewhere)

  type FailureCallback = (errorCode: number, errorMessage: string) => void;



  // Wrapper for the injected API to handle response parsing

  const apiWrapper = useMemo(() => {

    if (!api) {

      return undefined;

    }



    // Return an object with methods that wrap the raw API calls

    // These methods accept callbacks expecting PARSED data.

    return {

      getGames: (

        onSuccess: (games: Game[]) => void, // Wrapper expects callback with Game[]

        onFailure: FailureCallback

      ) => {

        api.getGameList(

          (gamesJson: string) => { // Add explicit type for raw param

            try {

              const parsedGames: Game[] = JSON.parse(gamesJson);

              if (Array.isArray(parsedGames)) {

                  onSuccess(parsedGames); // Call original success with parsed Game[] array

              } else {

                  console.error("Parsed getGames response is not an array:", parsedGames);

                  onFailure(999, "Invalid data format received (expected array)");

              }

            } catch (e) {

              console.error("Failed to parse getGames response:", e, "Raw:", gamesJson);

              onFailure(999, "Failed to parse response from backend");

            }

          },

          onFailure // Pass onFailure directly

        );

      },

      getVersion: (

        onSuccess: (version: string) => void, // Wrapper expects callback with string

        onFailure: FailureCallback

      ) => {

        api.getVersion(

          (versionJson: string) => { // Add explicit type for raw param

            try {

              let versionStr: string;

              try {

                const parsed = JSON.parse(versionJson);

                if (typeof parsed === 'object' && parsed !== null && typeof parsed.version === 'string') {

                    versionStr = parsed.version;

                } else {

                    versionStr = versionJson;

                }

              } catch (jsonError) {

                 versionStr = versionJson;

              }

              onSuccess(versionStr); // Call original success with the version string

            } catch (e) {

              console.error("Error processing getVersion response:", e, "Raw:", versionJson);

              onFailure(999, "Failed to process version response from backend");

            }

          },

          (errCode: number, errMsg: string) => {

             // Ensure onFailure is called with correct arguments if needed

             console.error('API failed to get version:', errCode, errMsg);

             onFailure(errCode, errMsg);

          }

        );

      },

      performGameAction: (

        action: 'install' | 'launch' | 'update' | 'verify' | 'repair' | 'cancel',

        gameId: string,

        onSuccess: () => void, // Wrapper expects callback with void

        onFailure: FailureCallback

      ) => {

        // Assuming this doesn't return data, just confirms success/failure

        api.performGameAction(action, gameId,

           () => { onSuccess(); },

           onFailure

        );

      },

      login: (
        payload: { username: string, password?: string }, // Keep password optional for now
        onSuccess: (result: AuthStatus) => void, // Callback expects AuthStatus
        onFailure: FailureCallback
      ) => {
        if (!api || !api.login) return onFailure(500, "API not available");
        console.log("[apiWrapper] Calling real login for user:", payload.username);
        api.login(
          payload.username, // Pass username
          payload.password ?? '', // Pass password (or empty string if undefined)
          (statusJson: string) => { // Real API Success
            try {
              const parsedStatus = JSON.parse(statusJson) as AuthStatus;
              onSuccess(parsedStatus);
            } catch (e) {
              console.error("[apiWrapper] Login success callback: Failed to parse JSON:", statusJson, e);
              onFailure(500, "Invalid response format from server");
            }
          },
          onFailure // Pass failure callback directly
        );
      },

      logout: (

        onSuccess: (status: AuthStatus) => void, // Wrapper expects callback with AuthStatus

        onFailure: FailureCallback

       ) => {

        api.logout(

           (statusJson: string) => { // Add explicit type for raw param

            try {

              const parsedStatus: AuthStatus = JSON.parse(statusJson);

              if (parsedStatus && parsedStatus.status === 'LoggedOut') {

                  onSuccess(parsedStatus); // Pass parsed status

              } else {

                  // Allow for potential variations if backend confirms differently

                  console.warn("Logout response status wasn't strictly 'LoggedOut':", parsedStatus);

                  onSuccess(parsedStatus); // Still treat as success if parseable

              }

            } catch (e) {

              console.error("Failed to parse logout response:", e, "Raw:", statusJson);

              onFailure(999, "Failed to parse logout response");

            }

           },

           onFailure

        );

      },

    };

  }, [api]); // Re-create wrapper only when the raw 'api' object changes



  // --- Initial Data Fetching (uses the inferred apiWrapper type) ---

  useEffect(() => {

    // Only run this effect if the apiWrapper is ready

    if (apiWrapper) {

      console.log("API Wrapper ready. Fetching initial data...");

      setIsLoadingGames(true); // Set loading true before fetch



      try {

        // Fetch Game List using the wrapper

        apiWrapper.getGames(

          (fetchedGames: Game[]) => { // Add explicit type

            // Type is already validated within the wrapper's onSuccess

            setGames(fetchedGames);

            // Select first game if list isn't empty and none is selected

            if (fetchedGames.length > 0 && !selectedGameId) {

              setSelectedGameId(fetchedGames[0].id);

            }

          },

          (errCode: number, errMsg: string) => { // Add explicit types

            console.error("Failed to fetch initial game list:", errCode, errMsg);

            setApiError(`Failed to load games: ${errMsg} (${errCode})`);

            setGames([]); // Ensure games is empty on error

          }

        );



        // Fetch Version using the wrapper

        apiWrapper.getVersion(

          (version: string) => { // Add explicit type

            console.log("[Initial Fetch] Version Success:", version);

            // setVersion(version); // Update state

          },

          (errCode: number, errMsg: string) => { // Add explicit types

            console.error("Failed to fetch initial version:", errCode, errMsg);

            setApiError(`Failed to get version: ${errMsg} (${errCode})`);

          }

        );



      } catch (error: any) {

        console.error("[Initial Fetch] Synchronous error during initial data fetch:", error);

        setApiError('An unexpected error occurred while fetching initial data.');

        setGames([]);

      } finally {

        // Set loading false after all initial fetches are attempted

        setIsLoadingGames(false);

      }

    } else if (api === null && !isLoadingGames) {

      // This handles the case where the initial API check finished and found nothing

      // We only set loading false here if it wasn't already set by the apiWrapper block

      setIsLoadingGames(false);

    }



  }, [apiWrapper, selectedGameId]); // Add apiWrapper and selectedGameId dependencies



  // Handler for login action

  const handleLoginSubmit = async (username: string): Promise<void> => {
    console.log(`Initiating Username/Password Login for ${username} via Auth0 popup...`);
    setLoginError(null); // Clear previous errors
    // isLoadingAuth should be managed by useAuth0's isLoading state, but we can track popup-specific loading if needed.

    try {
      await loginWithPopup({
        authorizationParams: {
          connection: 'Username-Password-Authentication', // Verify this connection name in Auth0 Dashboard
          login_hint: username, // Pre-fill username in Auth0 form
          // screen_hint: 'signup' // Optional: force signup screen
        }
      });
      console.log('Auth0 Popup login successful.');
      // State (isAuthenticated, user) should be updated automatically by the SDK after popup success.
    } catch (error: any) {
      console.error('Auth0 Popup login failed:', error);
      // Handle specific errors, e.g., user closing the popup
      if (error.error === 'popup_closed') {
        setLoginError('Login cancelled: Popup was closed.');
      } else {
        setLoginError(error.error_description || error.message || 'Login failed. Please try again.');
      }
    }
    // No need to manually set isLoadingAuth false here, SDK handles it.
  };

  // Handler for Auth0 Google Login
  const handleLoginWithGoogle = () => {
    console.log('Initiating Google Login via Auth0 redirect...');
    // Ensure loginWithRedirect is called correctly from useAuth0
    loginWithRedirect({ 
      authorizationParams: {
        connection: 'google-oauth2',
        // Optionally add screen_hint: 'signup' or 'login' if needed
      },
      appState: { returnTo: window.location.pathname } // Optional: store current path to return after login
    });
  };

  // Handler for Auth0 Logout
  const handleLogout = useCallback(() => {
    console.log('Initiating logout via Auth0...');
    logout({ logoutParams: { returnTo: window.location.origin } });
  }, [logout]);

  const handleSelectGame = useCallback((gameId: string | null) => {
    setSelectedGameId(gameId);
  }, []);

  // Handler for game actions (install, launch, update)

  const handleGameAction = (gameId: string, action: string) => {

    // Basic validation for action type if needed, though TypeScript helps

    const validActions = ['install', 'launch', 'update', 'verify', 'repair', 'cancel'];

    if (!validActions.includes(action)) {

        console.error(`Invalid game action requested: ${action}`);

        return;

    }



    if (apiWrapper) {

      console.log(`Performing action '${action}' on game '${gameId}'`);

      // Perform optimistic UI update immediately (e.g., show 'Starting install...')

      // Find the game and update its status optimistically if appropriate

      setGames(prevGames => prevGames.map(g => {

          if (g.id === gameId) {

              let optimisticStatus: Game['status'] = g.status; // Keep current status by default

              switch (action) {

                  case 'install': optimisticStatus = 'Downloading'; break; // Or 'Starting install'

                  case 'update': optimisticStatus = 'Downloading'; break; // Or 'Starting update'

                  case 'launch': /* No immediate status change, maybe show 'Launching...' briefly elsewhere */ break;

                  case 'verify': optimisticStatus = 'Verifying'; break;

                  case 'repair': optimisticStatus = 'Repairing'; break;

                  case 'cancel':

                      // Revert status based on what was being cancelled?

                      // This is tricky without knowing the previous state.

                      // Maybe just revert to 'NotInstalled' or 'ReadyToPlay' if applicable.

                      // For now, just log it.

                      console.log(`Cancelling action for ${gameId}`);

                      break;

              }

              return { ...g, status: optimisticStatus };

          }

          return g;

      }));



      apiWrapper.performGameAction(

        action as 'install' | 'launch' | 'update' | 'verify' | 'repair' | 'cancel', // Keep cast

        gameId,

        () => { // Add explicit type (void)

          console.log(`Action '${action}' on game '${gameId}' succeeded.`);

          // Backend should send GameUpdateEvent for actual progress/status changes.

          // We might not need to do anything here if events handle the final state.

          // Optional: Refetch game list or specific game status if no events are used for confirmation.

        },

        (errCode: number, errMsg: string) => { // Add explicit types

          console.error(`Action '${action}' on game '${gameId}' failed:`, errCode, errMsg);

          setApiError(`Action '${action}' failed: ${errMsg} (${errCode})`);

          // Revert optimistic UI update

          // Refetch game status to get the actual state after failure

          if (apiWrapper.getGames) { // Check if method exists before calling

            apiWrapper.getGames((games: Game[]) => setGames(games), (code: number, msg: string) => { // Add explicit types

              console.error("Failed to refetch game list after action failure:", code, msg);

            });

          }

        }

      );

    } else {

      setApiError("API not available.");

      console.error(`Action '${action}' attempt failed: API wrapper not available.`);

    }

  };



  // --- New Navigation Handler ---

  const handleNavClick = (view: string) => {

    console.log("Navigating to view:", view);

    setActiveView(view);

  };



  const selectedGame = useMemo(() => {

    return games.find(game => game.id === selectedGameId) || null;

  }, [games, selectedGameId]);



  // --- Render Logic ---

  console.log('[App] Rendering with authStatus:', isAuthenticated); // <-- Update log

  return (
    <div>
      <Layout
        activeView={activeView}
        setActiveView={handleNavClick}
        games={games}
        selectedGame={selectedGame}
        selectedGameId={selectedGameId} // Added missing prop
        setSelectedGameId={handleSelectGame}
        handleGameAction={handleGameAction}
        isLoading={isLoadingGames || isLoadingAuth}
        error={apiError || (authError ? authError.message : null) || loginError}
        version={version}
        // --- Authentication Props ---
        isAuthenticated={isAuthenticated}
        user={user}
        isLoadingAuth={isLoadingAuth} // Added missing prop
        isLoginModalOpen={isLoginModalOpen}
        onOpenLoginModal={() => setIsLoginModalOpen(true)}
        onCloseLoginModal={() => setIsLoginModalOpen(false)}
        onLoginSubmit={handleLoginSubmit}
        onLoginWithGoogle={handleLoginWithGoogle}
        onLogout={handleLogout}
        loginError={loginError} // Added missing prop
      />
      {version && <div className="fixed bottom-1 right-1 text-xs text-gray-500">v{version}</div>}
    </div>
  );

};

export default App;