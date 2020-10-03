'use strict'
import { app, protocol, BrowserWindow, ipcMain, Menu } from 'electron'
import { createProtocol, installVueDevtools } from 'vue-cli-plugin-electron-builder/lib'
import path from 'path';
import Bonjour from 'bonjour';
import * as Splashscreen from "@trodi/electron-splashscreen";
import { applicationMenu } from './menu';
import { Flasher } from './flasher';


const isDevelopment = process.env.NODE_ENV !== 'production'
declare const __static: string;

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
let win: BrowserWindow | null

// Scheme must be registered before the app is ready
protocol.registerSchemesAsPrivileged([
  { scheme: 'app', privileges: { secure: true, standard: true } }
])

Menu.setApplicationMenu(applicationMenu())

// browse for all http services
const serviceDiscovery = new Bonjour();

function createWindow() {
  // Create the browser window.
  const mainOpts: Electron.BrowserWindowConstructorOptions = {
    width: 1024,
    height: 768,
    icon: path.join(__static, 'icon.png'),
    webPreferences: {
      // Use pluginOptions.nodeIntegration, leave this alone
      // See nklayman.github.io/vue-cli-plugin-electron-builder/guide/security.html#node-integration for more info
      nodeIntegration: (process.env
          .ELECTRON_NODE_INTEGRATION as unknown) as boolean
    },
  };

  let minTime = 1500;
  if (isDevelopment) {
    minTime= 0;
  }

  const config: Splashscreen.Config = {
    windowOpts: mainOpts,
    templateUrl: path.join(__static, 'logo.svg'),
    delay: 0,
    minVisible: minTime,
    splashScreenOpts: {
      width: 600,
      height: 400,
      transparent: true
    }
  };


  win = Splashscreen.initSplashScreen(config);

  if (process.env.WEBPACK_DEV_SERVER_URL) {
    // Load the url of the dev server if in development mode
    win.loadURL(process.env.WEBPACK_DEV_SERVER_URL as string)
    if (!process.env.IS_TEST) win.webContents.openDevTools()
  } else {
    createProtocol('app')
    // Load the index.html when not in development
    win.loadURL('app://./index.html')
  }

  win.on('closed', () => {
    win = null
  })
}

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  // On macOS it is common for applications and their menu bar
  // to stay active until the user quits explicitly with Cmd + Q
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

app.on('activate', () => {
  // On macOS it's common to re-create a window in the app when the
  // dock icon is clicked and there are no other windows open.
  if (win === null) {
    createWindow()
  }
})

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', async () => {
  if (isDevelopment && !process.env.IS_TEST) {
    // Install Vue Devtools
    try {
      await installVueDevtools()
    } catch (e) {
      console.error('Vue Devtools failed to install:', e.toString())
    }
  }
  createWindow()
})

ipcMain.on('get-tallys', (event) => {
  const browser = serviceDiscovery.find({type: 'tallyblaster'}, () => {
    app.setBadgeCount(browser.services.length);
    event.reply('tally-nodes', browser.services);
  })
})

const Flash = new Flasher();
ipcMain.on('get-serial-devices', (event) => {
  Flash.getSupportedDevices().then(devices => {
    event.reply('detected-serial-devices', devices);
  })
})

ipcMain.on('get-firmware-downloads', (event) => {
  Flash.getFirmwares().then(files => {
    event.reply('downloaded-firmwares', files);
  })
})

ipcMain.on('download-firmware', (event, firmware) => {
  Flash.downloadFirmware(firmware).then(files => {
    console.log('--', files);
    event.reply('downloaded-firmwares', files);
  })
})

ipcMain.on('flash-node', (event, port, firmware) => {
  Flash.flashNode(port, firmware).then(() => {
    event.reply('flash-complete');
  })
})

// Exit cleanly on request from parent process in development mode.
if (isDevelopment) {
  if (process.platform === 'win32') {
    process.on('message', (data) => {
      if (data === 'graceful-exit') {
        app.quit()
      }
    })
  } else {
    process.on('SIGTERM', () => {
      app.quit()
    })
  }
}

