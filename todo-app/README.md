# To-Do List Application

A simple, elegant to-do list application with local storage functionality. All your tasks are automatically saved to your browser's local storage, so they persist even after you close the browser.

## Features

✅ **Add Tasks** - Easily add new tasks with the input field  
✅ **Local Storage** - Tasks are saved automatically and persist across sessions  
✅ **Mark Complete** - Click the checkbox to mark tasks as complete  
✅ **Delete Tasks** - Remove individual tasks or all completed tasks at once  
✅ **Filter Tasks** - View all tasks, active tasks, or completed tasks  
✅ **Task Counter** - See how many tasks remain incomplete  
✅ **Responsive Design** - Works great on desktop, tablet, and mobile devices  

## How to Use

1. **Open the app** - Open `index.html` in your web browser
2. **Add a task** - Type in the input field and click "Add Task" or press Enter
3. **Mark complete** - Click the checkbox next to a task to mark it as done
4. **Delete a task** - Click the "Delete" button to remove a task
5. **Filter tasks** - Use the filter buttons to view All, Active, or Completed tasks
6. **Clear completed** - Click "Clear Completed" to remove all finished tasks

## Files

- **index.html** - HTML structure and markup
- **styles.css** - Styling and responsive design
- **script.js** - JavaScript logic with local storage management
- **README.md** - This file

## Local Storage

The app uses the browser's `localStorage` API to persist your tasks:
- Tasks are stored under the key: `todoList`
- Data is stored as JSON
- Automatically saves after every action
- Loads automatically when you open the app

## Browser Compatibility

Works on all modern browsers that support:
- ES6 JavaScript
- Local Storage API
- Flexbox CSS

## Example Usage

```javascript
// The TodoApp class handles all functionality:
new TodoApp();

// Tasks are stored in localStorage as JSON:
// [{id: 1234567890, text: "Buy groceries", completed: false, createdAt: "..."}]
```

## Tips

- Press **Enter** to quickly add a task
- Use **Filter buttons** to focus on active or completed tasks
- Your tasks are saved locally, so they'll be there when you come back
- No internet connection needed - everything works offline!

## Future Enhancements

Possible improvements:
- Edit existing tasks
- Due dates and priorities
- Task categories/tags
- Dark mode
- Export/import tasks
- Recurring tasks
