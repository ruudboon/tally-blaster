import { shell, Menu } from 'electron';

const isMac = process.platform === 'darwin';

export const applicationMenu = () => {

    const helpMenu: Electron.MenuItemConstructorOptions = {
        role: 'help',
        submenu: [
            {
                label: 'Learn More',
                click: async () => {
                    await shell.openExternal('https://tally-blaster.ruudboon.io');
                }
            },
            {
                label: 'Buy me a beer',
                click: async () => {
                    await shell.openExternal('https://www.paypal.me/ruudboon');
                }
            },
            {
                label: 'Sponsor the project',
                click: async () => {
                    await shell.openExternal('https://github.com/sponsors/ruudboon');
                }
            },
            {
                label: 'Github Repository',
                click: async () => {
                    await shell.openExternal('https://github.com/ruudboon/tally-blaster');
                }
            },
            {
                label: 'Search Issues',
                click: async () => {
                    await shell.openExternal('https://github.com/ruudboon/tally-blaster/issues');
                }
            }
        ]
    };

    const macAppMenu: Electron.MenuItemConstructorOptions = { role: 'appMenu' };
    const template: Electron.MenuItemConstructorOptions[] = [
        ...(isMac ? [macAppMenu] : []),
        { role: 'fileMenu' },
        { role: 'editMenu' },
        { role: 'viewMenu' },
        { role: 'windowMenu' },
        helpMenu
    ];

    return Menu.buildFromTemplate(template);
};