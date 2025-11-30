
    /* --- JAVASCRIPT (script.js) --- */
    
    // 1. Select DOM Elements
    const taskInput = document.getElementById('task-input');
    const categorySelect = document.getElementById('category-select');
    const dateInput = document.getElementById('date-input');
    const addBtn = document.getElementById('add-btn');
    const taskList = document.getElementById('task-list');

    // 2. Load Tasks from LocalStorage on startup
    let tasks = JSON.parse(localStorage.getItem('myTasks')) || [];
    renderTasks();

    // 3. Add Task Function
    addBtn.addEventListener('click', () => {
        const text = taskInput.value;
        const category = categorySelect.value;
        const date = dateInput.value;

        if (text === '') {
            alert("Please enter a task name!");
            return;
        }
        if (date === '') {
            alert("Please pick a due date!");
            return;
        }

        const newTask = {
            id: Date.now(), // Unique ID based on time
            text: text,
            category: category,
            date: date
        };

        tasks.push(newTask);
        saveAndRender();
        
        // Clear inputs
        taskInput.value = '';
        dateInput.value = '';
    });

    // 4. Delete Task Function
    function deleteTask(id) {
        tasks = tasks.filter(task => task.id !== id);
        saveAndRender();
    }

    // 5. Save to LocalStorage and Update UI
    function saveAndRender() {
        localStorage.setItem('myTasks', JSON.stringify(tasks));
        renderTasks();
    }

    // 6. The Render Logic (Includes Reminder Check)
    function renderTasks() {
        taskList.innerHTML = ""; // Clear current list

        // Sort tasks by date (Earliest first)
        tasks.sort((a, b) => new Date(a.date) - new Date(b.date));

        const todayStr = new Date().toISOString().split('T')[0]; // Get today as YYYY-MM-DD

        tasks.forEach(task => {
            // Check if Due Today
            const isDueToday = (task.date === todayStr);
            const isOverdue = (task.date < todayStr);

            // Create HTML
            const li = document.createElement('li');
            
            // Add styling classes based on category and due date
            let cardClass = `task-card cat-${task.category}`;
            let reminderHTML = "";

            if (isDueToday) {
                cardClass += " due-today";
                reminderHTML = `<span class="due-alert">⚠️ Due Today!</span>`;
            } else if (isOverdue) {
                reminderHTML = `<span class="due-alert" style="color:red">Overdue</span>`;
            }

            li.innerHTML = `
                <div class="${cardClass}">
                    <div class="task-info">
                        <span class="task-title">${task.text}</span>
                        <div class="task-meta">
                            <span class="badge badge-${task.category}">${task.category}</span>
                            <span>📅 ${task.date}</span>
                            ${reminderHTML}
                        </div>
                    </div>
                    <button class="delete-btn" onclick="deleteTask(${task.id})">🗑️</button>
                </div>
            `;

            taskList.appendChild(li);
        });
    }