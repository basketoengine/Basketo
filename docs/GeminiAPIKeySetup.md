# Gemini API Key Setup Guide

## Overview

The Basketo Game Engine includes AI-powered features using Google's Gemini AI. To use these features, you need to configure your Gemini API key.

## 🚀 Quick Setup

### Step 1: Get Your Gemini API Key

1. **Visit**: [Google AI Studio](https://aistudio.google.com/app/apikey)
2. **Sign in** with your Google account
3. **Click "Create API Key"** 
4. **Copy** the generated API key

### Step 2: Configure in Engine

1. **Open** the Basketo Game Engine
2. **Navigate** to the AI Prompt panel (bottom panel)
3. **Look for** the "🤖 Gemini AI Configuration" section

#### If No API Key is Configured:
- You'll see "✗ Gemini API Key Required" in red
- An input field will be visible: "🔑 Enter Gemini API Key:"
- **Paste your API key** in the password field (shows as dots)
- **Click "Save"** to store the key
- **Click "Help"** for detailed instructions

#### If API Key is Already Configured:
- You'll see "✓ Gemini API Key Configured" in green
- **Click "Change Key"** to update your API key

## 🔧 Features

### Persistent Storage
- Your API key is automatically saved to `config.json`
- The key persists between engine restarts
- No need to re-enter the key every time

### Security
- API key input field is masked (shows as dots)
- Key is stored locally in your project directory
- Never transmitted except to Google's Gemini API

### Real-time Updates
- Changes take effect immediately
- No need to restart the engine
- Instant feedback on configuration status

## 🤖 AI Features Available

Once configured, you can use these AI features:

### Natural Language Commands
```
"create a player at 100 200"
"add a script to the player"
"move the enemy to 300 400"
```

### Script Generation
```
gemini_script create a player controller with jumping
```

### Entity Modification
```
gemini_modify player make it move faster
```

## 📁 Configuration Files

### config.json
The API key is stored in `config.json` in your project root:

```json
{
    "gemini_api_key": "your_api_key_here"
}
```

### Environment Variable (Alternative)
You can also set the `GEMINI_API_KEY` environment variable:

```bash
export GEMINI_API_KEY="your_api_key_here"
```

**Priority**: config.json > environment variable

## 🛠️ Troubleshooting

### "API Key Not Configured" Error
1. Check that you've entered the API key correctly
2. Verify the key is saved (should show green checkmark)
3. Try removing and re-adding the key

### "API Request Failed" Error
1. Check your internet connection
2. Verify your API key is valid at [Google AI Studio](https://aistudio.google.com/app/apikey)
3. Ensure you haven't exceeded API quotas

### Key Not Persisting
1. Check that the engine has write permissions to the project directory
2. Verify `config.json` exists and is writable
3. Check console for error messages

## 🔒 Security Best Practices

### Do NOT:
- Share your API key publicly
- Commit `config.json` with your API key to version control
- Use the same key across multiple projects in production

### DO:
- Keep your API key private
- Add `config.json` to your `.gitignore` file
- Use separate keys for development and production
- Monitor your API usage at [Google AI Studio](https://aistudio.google.com/app/apikey)

## 💡 Tips

1. **Free Tier**: Gemini offers a generous free tier for development
2. **Rate Limits**: Be aware of API rate limits for production use
3. **Backup**: Keep a backup of your API key in a secure location
4. **Multiple Keys**: You can create multiple API keys for different projects

## 🆘 Support

If you encounter issues:

1. Check the engine console for error messages
2. Verify your API key at Google AI Studio
3. Ensure you have an active internet connection
4. Try creating a new API key if the current one doesn't work

## 📚 Additional Resources

- [Google AI Studio](https://aistudio.google.com/)
- [Gemini API Documentation](https://ai.google.dev/docs)
- [API Key Management](https://aistudio.google.com/app/apikey)
