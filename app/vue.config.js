module.exports = {
  "transpileDependencies": [
    "vuetify"
  ],
  pluginOptions: {
    electronBuilder: {
      // List native deps here if they don't work
      // If you are using Yarn Workspaces, you may have multiple node_modules folders
      // List them all here so that VCP Electron Builder can find them
      nodeModulesPath: [
          '../../node_modules',
          './node_modules'
      ],
      nodeIntegration: true,
      files: [
        "build/*"
      ],
      builderOptions: {
        appId: "io.ruudboon.tally-blaster",
        productName: "Tally Blaster Control App",
        afterSign: "electron-builder-notarize",
        publish: ['github'],
        mac: {
          category: "public.app-category.video",
          entitlements: "./build/entitlements.mac.plist",
          icon: './build/icons/icon.icns',
          hardenedRuntime: true,
          // provisioningProfile: "./build/Tally_Blaster.provisionprofile",
        },
        dmg: {
          icon: false
        },
        linux: {
          icon: './public/icon.png',
          category: "Development",
          target: [
              'snap',
              'deb',
              'appImage'
          ],
          category: "Video"
        }
      }
    }
  },
  publicPath: process.env.NODE_ENV === 'production' ? './' : '/'
}