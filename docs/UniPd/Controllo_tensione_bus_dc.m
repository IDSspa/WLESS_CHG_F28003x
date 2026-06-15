function [P_bat_rif, I_L_rif_A, duty_cycle_A, duty_cycle_B] = Controllo_Tensione_Bus_dc(V_dc_rif, V_dc, V_bat, I_L_A, I_L_B, I_bat_rif_max, I_bat_rif_min)

% Nella realizzazione in C questo codice è parte di una routine di servizio
% di un interrupt per cui non ci saranno varibili di ingreso e uscita e la
% funzione dovrà operare su variabili condivise con le altre sezioni del
% firmware.
% Tutte le grandezze sono supposte di tipo float e sono scalate nelle unità
% naturali (Volt, Ampere,...).
% Nel protipo realizzato a Padova la routine dei serviio veniva eseguita
% come risposta ad un interrup generato alla fine della conversione
% analogico-digitale delle varie grandezze di interesse. Di conseguenza
% quando la routine viene eseguita tutti i risultati della conversione sono
% già disponibili. A sua volta, la conversione analogico-digitale è
% effettuata all'inizio del periodo di PWM.

% Al fine di garantire un tempo di eleborazione sufficiente alla routine di
% servizio si è supposto che essa venga eseguita con una frequenza pari a
% 1/4 di quella di alimentazione delle bobine. (85 kHz/4 = 21.25 kHz).
% Quindi il periodo di campionamento risulta di circa 47 us.
% La medesima routine di servizion deve gestire sia il convertitore dc-dc
% che il convertitore dc-ac. La frequenza di switching del primo era stata
% fissata a 120 kHz come nel prototipo Texas. questa frequenza non è multipla
% di quella di campionamento per cui si propone di alzare la frequenza di
% switching del convertitore dc-dc a 6 x 21.25 kHz = 127.5 kHz. In questo
% modo non sarà necessario ridimensionare le induttanze di filtro collegate
% all'ingresso del convertitore dc-dc.

% Varibili in ingresso
% V_dc, V_bat, I_L_A e I_L_B sono rispettivamente i valori attuali della
% tensione del bus_ dc, della tensione di batteria e della corrente nei due
% rami del convertitore buck-boost dc-dc. Sono valori ottenuti scalando in
% maniera opportuna le uscite fornite dai convertitori analogico/digitali.
% V_dc_rif è il riferimento per la tensione del bus dc. Durante il
% funzionamento è costante e assume due valori diversi a seconda che la
% batteria venga caricata o scaricata.
% I_bat_rif_max e I_bat_rif_min sono i limiti di corrente per la batteria
% forniti dal BMS.

% Variabili in uscita
% duty_cycle_A e duty_Cycle_B sono i duty cycle per i due rami del
% convertitore buck-boost dc-dc. % I duty cycle sono compresi tra 0 e 1.
% Vanno scalati opprotunamente per essere impiegati nella periferica del
% microcontrollore che genera i comandi di PWM.
% P_bat_rif e I_L_rif_A sono il riferimento della potenza da estrarre o
% iniettare nella batteria e della corrente nei due rami del convertitore
% buck-boost. Sono usate nella simulazione solo per visualizzazione, nella
% realizzazione in C non è necessario condividerle con altre sezioni del
% firmware.


% Costanti di controllo
% Possono essere delle varibili globali definite in un altro file oppure
% delle costanti definite in un file estrerno tramite #define e poi
% importate tramite #include
% Sono dei limiti fittizi imposti alle tensioni in modo che l'agoritmo di
% controllo non si trovi a lavorare con valori troppo grandi o troppo
% piccoli. Nel sistema reale non dovrebbe essere necessario gestire queste
% limitazioni
V_bat_max_alg=218;
V_bat_min_alg=37.5;
V_dc_max_alg=500;
V_dc_min_alg=62.5;

% Parametri dei controllori
% Controllore della corrente nei due rami del convertitore dc-dc

% Banda Passante 250 Hz. fs=21.25 kHz (85 kHz/4)
K_I_L_err=0.533694299666651;    % Guadagno applicato all'errore di corrente attuale
K_I_L_err_p=-0.526383425589328; % Guadagno applicato all'errore di corrente relativo all'istante di campionamento precedente


% Controllore della tensione del bus dc
% Banda Passante 10 Hz, fs=21.25 kHz (85 kHz/4)
K_V_dc_err=0.046777788128766;   % Guadagno applicato all'errore di tensione attuale
K_V_dc_err_p=-0.046768069962416;    % Guadagno applicato all'errore di tensione precedente


% Variabili relative all'istante di campionamento precedente.
% Devono essere mantenute tra una chiamata e l'altra della routine di
% servio.
% Sono equivalenti alle varibili "static" in C
persistent Inizializzato
persistent I_L_err_A_p
persistent I_L_err_B_p % Errori di corrente nei due rami del convertitore buck-boost al passo di campionamento precedente
persistent V_L_rif_A_p
persistent V_L_rif_B_p % Riferimenti di tensione da applicare alle induttanza di filtro del buck-boost al passo precedente 
persistent V_dc_2_err_p % Errore di tensione al passo precedente 
persistent P_bat_rif_p % Riferimento di potenza al passo precedente 

% Inizizzazione delle varibili persistent
% In C corrisponde ad assegnare il valore iniziale alla variabili statiche
% al momento della dichiarazione, per cui non serve una struttura "if"
if isempty(Inizializzato)
    Inizializzato = 1;
    I_L_err_A_p = 0;
    I_L_err_B_p = 0;
    V_L_rif_A_p = 0;
    V_L_rif_B_p = 0;
    V_dc_2_err_p = 0;
    P_bat_rif_p = 0;
end

% Limita i valori misurati di V_bat e V_dc per evitare problemi di calcolo
% (nell'applicazione normale non dovrebbe mai succedere)

if(V_bat>V_bat_max_alg)
    V_bat=V_bat_max_alg;
end    

if(V_bat<V_bat_min_alg)
    V_bat=V_bat_min_alg;
end

if(V_dc>V_dc_max_alg)
    V_dc=V_dc_max_alg;
end    

if(V_dc<V_dc_min_alg)
    V_dc=V_dc_min_alg;
end

% Limiti di potenza trasferibile
P_bat_rif_max=V_bat * I_bat_rif_max;
P_bat_rif_min=V_bat * I_bat_rif_min;

% Errore tra riferimento al quadrato di tensione e tensione attuale al quadrato
V_dc_2_err = V_dc_rif*V_dc_rif - V_dc*V_dc;

% Controllore di tensione. Calcola la potenza da iniettare nel condensatore del bus dc
P_bat_rif = P_bat_rif_p + K_V_dc_err*V_dc_2_err + K_V_dc_err_p*V_dc_2_err_p;


% Limitazione del riferimento di potenza
if(P_bat_rif>P_bat_rif_max)
    P_bat_rif=P_bat_rif_max;
end    

if(P_bat_rif<P_bat_rif_min)
    P_bat_rif=P_bat_rif_min;
end    

% Calcolo del riferimento di corrente di batteria a partire dal riferimento di potenza
I_bat_rif = P_bat_rif/V_bat;

% Calcolo dei riferimenti per le correnti nei due rami del convertitore buck-boost
% I due riferimento sono uguali per i due rami 
I_L_rif_A=0.5*I_bat_rif;
I_L_rif_B=I_L_rif_A;

% Controllori di corrente
% Errori di corrente
I_L_err_A=I_L_rif_A-I_L_A;
I_L_err_B=I_L_rif_B-I_L_B;


% Valori estremi della tensione applicabile sulle induttanze di filtro
V_L_rif_max=V_bat;      % Corrisponde ad avere il transistor "basso" sempre chiuso
V_L_rif_min=V_bat-V_dc; % Corrisponde ad avere il transistor "alto" sempre chiuso

% Controllori di corrente. Calcolano la tensione da applicare sulle induttanze di filtro
V_L_A_rif = V_L_rif_A_p + K_I_L_err*I_L_err_A + K_I_L_err_p*I_L_err_A_p;
V_L_B_rif = V_L_rif_B_p + K_I_L_err*I_L_err_B + K_I_L_err_p*I_L_err_B_p;


% Limitazione dei riferimenti per evitare windup (il duty cycle non può essere maggiore di 1)
if(V_L_A_rif>V_L_rif_max)
    V_L_A_rif=V_L_rif_max;
end 

if(V_L_A_rif<V_L_rif_min)
    V_L_A_rif=V_L_rif_min;
end 

if(V_L_B_rif>V_L_rif_max)
    V_L_B_rif=V_L_rif_max;
end 

if(V_L_B_rif<V_L_rif_min)
    V_L_B_rif=V_L_rif_min;
end 

% Tensioni da generare all'ingresso (punto di mezzo dei due transistor) del buck-boost
V_buckboost_A = V_bat - V_L_A_rif;
V_buckboost_B = V_bat - V_L_B_rif;

% Duty cycle corrispondenti alle tensioni da generare
duty_cycle_A = V_buckboost_A / V_dc;
duty_cycle_B = V_buckboost_B / V_dc;

% Aggiornamento delle variabili da usare nel prossimo istante di campionamento
I_L_err_A_p=I_L_err_A;
I_L_err_B_p=I_L_err_B;
V_L_rif_A_p=V_L_A_rif;
V_L_rif_B_p=V_L_B_rif;
V_dc_2_err_p = V_dc_2_err;
P_bat_rif_p = P_bat_rif;