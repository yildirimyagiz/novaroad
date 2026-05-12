/**
 * Nova Language Extension for VS Code
 */

import * as vscode from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
    TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: vscode.ExtensionContext) {
    console.log('Nova Language Extension activated');

    // Get configuration
    const config = vscode.workspace.getConfiguration('nova');
    const lspEnabled = config.get<boolean>('lsp.enable', true);
    
    if (!lspEnabled) {
        console.log('Nova LSP is disabled');
        return;
    }

    // LSP server path
    const serverPath = config.get<string>('lsp.path', 'znlsp');

    // Server options
    const serverOptions: ServerOptions = {
        command: serverPath,
        args: ['--stdio'],
        transport: TransportKind.stdio
    };

    // Client options
    const clientOptions: LanguageClientOptions = {
        documentSelector: [
            { scheme: 'file', language: 'nova' },
            { scheme: 'file', pattern: '**/*.zn' },
            { scheme: 'file', pattern: '**/*.nova' }
        ],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.{zn,nova}')
        },
        outputChannelName: 'Nova Language Server',
        traceOutputChannel: vscode.window.createOutputChannel('Nova LSP Trace')
    };

    // Create language client
    client = new LanguageClient(
        'nova-lsp',
        'Nova Language Server',
        serverOptions,
        clientOptions
    );

    // Start client
    client.start();

    // Register commands
    context.subscriptions.push(
        vscode.commands.registerCommand('nova.restartLanguageServer', async () => {
            vscode.window.showInformationMessage('Restarting Nova Language Server...');
            await client.stop();
            await client.start();
            vscode.window.showInformationMessage('Nova Language Server restarted');
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('nova.showServerStatus', () => {
            const status = client.isRunning() ? 'Running' : 'Stopped';
            vscode.window.showInformationMessage(`Nova Language Server: ${status}`);
        })
    );

    // Format on save
    if (config.get<boolean>('format.onSave', true)) {
        context.subscriptions.push(
            vscode.workspace.onWillSaveTextDocument(event => {
                if (event.document.languageId === 'nova') {
                    event.waitUntil(
                        vscode.commands.executeCommand('editor.action.formatDocument')
                    );
                }
            })
        );
    }

    // Status bar item
    const statusBarItem = vscode.window.createStatusBarItem(
        vscode.StatusBarAlignment.Right,
        100
    );
    statusBarItem.text = '$(rocket) Nova';
    statusBarItem.tooltip = 'Nova Language Server';
    statusBarItem.command = 'nova.showServerStatus';
    statusBarItem.show();
    context.subscriptions.push(statusBarItem);

    console.log('Nova Language Server started');
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) {
        return undefined;
    }
    return client.stop();
}
