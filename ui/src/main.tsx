import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { Auth0Provider } from '@auth0/auth0-react'
import './index.css'
import App from './App.tsx'

const auth0Domain = "dev-yc2gaw2xjoct4fbx.us.auth0.com";
const auth0ClientId = "zHBTfeD9p02awRd4RZDouDJ45rJB979k";

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <Auth0Provider
      domain={auth0Domain}
      clientId={auth0ClientId}
      authorizationParams={{
        redirect_uri: window.location.origin 
        // Add audience and scope if needed for API access later
        // audience: "YOUR_API_IDENTIFIER",
        // scope: "openid profile email read:messages"
      }}
    >
      <App />
    </Auth0Provider>
  </StrictMode>,
)
